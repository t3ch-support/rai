/*  ------------------------------------------------------------------
    Copyright (c) 2011-2020 Marc Toussaint
    email: toussaint@tu-berlin.de

    This code is distributed under the MIT License.
    Please see <root-path>/LICENSE for details.
    --------------------------------------------------------------  */

#include "kin_bullet.h"

#ifdef RAI_BULLET

#include "frame.h"
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

// ============================================================================

constexpr float gravity = -9.8f;

// ============================================================================

void btTrans2raiTrans(rai::Transformation& f, const btTransform& pose) {
  const btQuaternion q = pose.getRotation();
  const btVector3& p = pose.getOrigin();
  f.pos.set(p.x(), p.y(), p.z());
  f.rot.set(q.w(), q.x(), q.y(), q.z());
}

btTransform conv_raiTrans2btTrans(const rai::Transformation& fX) {
  btTransform pose(btQuaternion(fX.rot.x, fX.rot.y, fX.rot.z, fX.rot.w),
                   btVector3(fX.pos.x, fX.pos.y, fX.pos.z));
  return pose;
}

arr conv_btVec3_arr(const btVector3& v) {
  return ARR(v.x(), v.y(), v.z());
}

btVector3 conv_arr_btVec3(const arr& v) {
  CHECK_EQ(v.N, 3, "");
  return btVector3(v.elem(0), v.elem(1), v.elem(2));
}

// ============================================================================

struct BulletInterface_self {
  btDefaultCollisionConfiguration* collisionConfiguration;
  btCollisionDispatcher* dispatcher;
  btBroadphaseInterface* broadphase;
  btSequentialImpulseConstraintSolver* solver;
  btDiscreteDynamicsWorld* dynamicsWorld;
  btAlignedObjectArray<btCollisionShape*> collisionShapes;

  btSoftBodyWorldInfo softBodyWorldInfo;

  rai::Array<btCollisionObject*> actors;
  rai::Array<rai::BodyType> actorTypes;

  rai::Bullet_Options opt;

  uint stepCount=0;

  btRigidBody* addGround(bool yAxisGravity=false);
  btRigidBody* addLink(rai::Frame* f, int verbose);
  btSoftBody* addSoft(rai::Frame* f, int verbose);

  btCollisionShape* createCollisionShape(rai::Shape* s);
  btCollisionShape* createCompoundCollisionShape(rai::Frame* link, ShapeL& shapes);
};

// ============================================================================

BulletInterface::BulletInterface(rai::Configuration& C, int verbose, bool yAxisGravity, bool enableSoftBodies) : self(nullptr) {
  self = new BulletInterface_self;

  if(verbose>0) LOG(0) <<"starting bullet engine ...";

  self->collisionConfiguration = new btDefaultCollisionConfiguration();
  self->dispatcher = new btCollisionDispatcher(self->collisionConfiguration);
  self->broadphase = new btDbvtBroadphase();
  //m_broadphase = new btAxisSweep3(worldAabbMin, worldAabbMax, maxProxies);
  self->solver = new btSequentialImpulseConstraintSolver;

  if(enableSoftBodies){
    self->dynamicsWorld = new btSoftRigidDynamicsWorld(self->dispatcher, self->broadphase, self->solver, self->collisionConfiguration);
  }else{
    self->dynamicsWorld = new btDiscreteDynamicsWorld(self->dispatcher, self->broadphase, self->solver, self->collisionConfiguration);
  }

  if(yAxisGravity){
    self->dynamicsWorld->setGravity(btVector3(0, gravity, 0));
    self->softBodyWorldInfo.m_gravity.setValue(0, gravity, 0);
  }else{
    self->dynamicsWorld->setGravity(btVector3(0, 0, gravity));
    self->softBodyWorldInfo.m_gravity.setValue(0, 0, gravity);
  }

  self->softBodyWorldInfo.m_dispatcher = self->dispatcher;
  self->softBodyWorldInfo.m_broadphase = self->broadphase;
  self->softBodyWorldInfo.m_sparsesdf.Initialize();
  self->softBodyWorldInfo.air_density = (btScalar)1.2;
  self->softBodyWorldInfo.water_density = 0;
  self->softBodyWorldInfo.water_offset = 0;
  self->softBodyWorldInfo.water_normal = btVector3(0, 0, 0);

  if(verbose>0) LOG(0) <<"... done starting bullet engine";

  self->addGround(yAxisGravity);

  if(verbose>0) LOG(0) <<"creating Configuration within bullet ...";

  self->actors.resize(C.frames.N); self->actors.setZero();
  self->actorTypes.resize(C.frames.N); self->actorTypes.setZero();
  FrameL links = C.getLinks();
  for(rai::Frame* a : links){
    if(a->inertia && a->inertia->type==rai::BT_soft){
      self->addSoft(a, verbose);
    }else{
      self->addLink(a, verbose);
    }
  }

  if(verbose>0) LOG(0) <<"... done creating Configuration within bullet";
}

BulletInterface::~BulletInterface() {
  for(int i = self->dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
    btCollisionObject* obj = self->dynamicsWorld->getCollisionObjectArray()[i];
    btRigidBody* body = dynamic_cast<btRigidBody*>(obj);
    if(body && body->getMotionState()) {
      delete body->getMotionState();
    }
    self->dynamicsWorld->removeCollisionObject(obj);
    delete obj;
  }
  for(int i = 0; i < self->collisionShapes.size(); ++i) {
    delete self->collisionShapes[i];
  }
  delete self->dynamicsWorld;
  delete self->solver;
  delete self->broadphase;
  delete self->dispatcher;
  delete self->collisionConfiguration;
  self->collisionShapes.clear();
}

void BulletInterface::step(double tau) {
  self->stepCount++;
  self->dynamicsWorld->stepSimulation(tau);
}

void pullPoses(FrameL& frames, const rai::Array<btCollisionObject*>& actors, arr& frameVelocities, bool alsoStaticAndKinematic){
  if(!!frameVelocities) frameVelocities.resize(frames.N, 2, 3).setZero();

  for(rai::Frame* f : frames) {
    if(actors.N <= f->ID) continue;
    btRigidBody* b = dynamic_cast<btRigidBody*>(actors(f->ID));
    if(b){
      if(alsoStaticAndKinematic || !b->isStaticOrKinematicObject()){
        rai::Transformation X;
        btTransform pose;
        if(b->getMotionState()) {
          b->getMotionState()->getWorldTransform(pose);
        } else {
          NIY; //trans = obj->getWorldTransform();
        }
        btTrans2raiTrans(X, pose);
        f->set_X() = X;
        if(!!frameVelocities) {
          frameVelocities(f->ID, 0, {}) = conv_btVec3_arr(b->getLinearVelocity());
          frameVelocities(f->ID, 1, {}) = conv_btVec3_arr(b->getAngularVelocity());
        }
      }
    }
    btSoftBody* softbody = dynamic_cast<btSoftBody*>(actors(f->ID));
    if(softbody){
      rai::Mesh &m = f->shape->mesh();
      CHECK_EQ(m.V.d0, softbody->m_nodes.size(), "");
      for(int i=0; i<softbody->m_nodes.size(); i++){
        m.V[i] = conv_btVec3_arr(softbody->m_nodes[i].m_x);
      }
    }
  }
}

void BulletInterface::pullDynamicStates(FrameL& frames, arr& frameVelocities) {
  pullPoses(frames, self->actors, frameVelocities, false);
}

void BulletInterface::changeObjectType(rai::Frame* f, int _type, const arr& withVelocity) {
  rai::Enum<rai::BodyType> type((rai::BodyType)_type);
  if(self->actorTypes(f->ID) == type) {
    //LOG(-1) <<"frame " <<*f <<" is already of type " <<type;
  }

  btRigidBody* a = dynamic_cast<btRigidBody*>(self->actors(f->ID));
  if(!a) HALT("frame " <<*f <<"is not an actor");

  if(type==rai::BT_kinematic) {
    a->setCollisionFlags(a->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    a->setActivationState(DISABLE_DEACTIVATION);
  } else if(type==rai::BT_dynamic) {
    a->setCollisionFlags(a->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
    a->setActivationState(DISABLE_DEACTIVATION);
    if(withVelocity.N){
      a->setLinearVelocity(btVector3(withVelocity(0), withVelocity(1), withVelocity(2)));
    }
  } else NIY;
  self->actorTypes(f->ID) = type;
}

void BulletInterface::pushKinematicStates(const FrameL& frames) {

  for(rai::Frame* f: frames) {
    if(self->actors.N <= f->ID) continue;
    if(self->actorTypes(f->ID)==rai::BT_kinematic) {
      btRigidBody* b = dynamic_cast<btRigidBody*>(self->actors(f->ID));
      if(!b) continue; //f is not an actor

      CHECK(b->getMotionState(), "");
      b->getMotionState()->setWorldTransform(conv_raiTrans2btTrans(f->ensure_X()));
    }
  }
}

void BulletInterface::pushFullState(const FrameL& frames, const arr& frameVelocities) {
  for(rai::Frame* f : frames) {
    if(self->actors.N <= f->ID) continue;
    btRigidBody* b = dynamic_cast<btRigidBody*>(self->actors(f->ID));
    if(!b) continue; //f is not an actor

    b->setWorldTransform(conv_raiTrans2btTrans(f->ensure_X()));
    b->setActivationState(ACTIVE_TAG);
    if(self->actorTypes(f->ID)==rai::BT_dynamic) {
      b->clearForces();
      if(!!frameVelocities && frameVelocities.N) {
        b->setLinearVelocity(btVector3(frameVelocities(f->ID, 0, 0), frameVelocities(f->ID, 0, 1), frameVelocities(f->ID, 0, 2)));
        b->setAngularVelocity(btVector3(frameVelocities(f->ID, 1, 0), frameVelocities(f->ID, 1, 1), frameVelocities(f->ID, 1, 2)));
      } else {
        b->setLinearVelocity(btVector3(0., 0., 0.));
        b->setAngularVelocity(btVector3(0., 0., 0.));
      }
    }
  }
  self->dynamicsWorld->stepSimulation(.01); //without this, two consequtive pushFullState won't work! (something active tag?)
}

btRigidBody* BulletInterface_self::addGround(bool yAxisGravity) {
  btTransform groundTransform;
  groundTransform.setIdentity();
  groundTransform.setOrigin(btVector3(0, 0, 0));
  btCollisionShape* groundShape;
  if(yAxisGravity){
    groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
  }else{
    groundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 0);
  }
  collisionShapes.push_back(groundShape);
  btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(0, myMotionState, groundShape, btVector3(0, 0, 0));
  btRigidBody* body = new btRigidBody(rbInfo);
  body->setRestitution(opt.defaultRestitution);
  body->setFriction(opt.defaultFriction);
  dynamicsWorld->addRigidBody(body);
  return body;
}

btRigidBody* BulletInterface_self::addLink(rai::Frame* f, int verbose) {
  //-- collect all shapes of that link
  ShapeL shapes;
  {
    FrameL tmp = {f};
    f->getRigidSubFrames(tmp);
    for(rai::Frame* p: tmp){
      if(p->shape
         && p->getShape().type()!=rai::ST_marker
         && p->getShape().alpha()==1.) shapes.append(p->shape);
    }
  }

  //-- check inertia
  bool shapesHaveInertia=false;
  for(rai::Shape *s:shapes) if(s->frame.inertia){ shapesHaveInertia=true; break; }
  if(shapesHaveInertia && !f->inertia){
    f->computeCompoundInertia();
    if(!f->inertia->com.isZero){
      CHECK(!f->shape || f->shape->type()==rai::ST_marker, "can't translate this frame if it has a shape attached");
      f->set_X()->pos += f->ensure_X().rot * f->inertia->com;
      for(rai::Frame* ch:f->children) ch->set_Q()->pos -= f->inertia->com;
      f->inertia->com.setZero();
//MISSING HERE: ALSO TRANSFORM THE INERTIA MATRIX TO BECOME DIAG!
    }
  }

  //-- decide on the type
  rai::BodyType type = rai::BT_static;
  if(shapes.N) {
    if(f->joint)   type = rai::BT_kinematic;
    if(f->inertia) type = f->inertia->type;
  }
  actorTypes(f->ID) = type;
  if(verbose>0) LOG(0) <<"adding link anchored at '" <<f->name <<"' as " <<rai::Enum<rai::BodyType>(type);

  //-- create a bullet collision shape
  btCollisionShape* colShape = 0;
  if(shapes.N==1 && f == &shapes.scalar()->frame) {
    colShape = createCollisionShape(shapes.scalar());
  } else {
    colShape = createCompoundCollisionShape(f, shapes);
  }
  collisionShapes.push_back(colShape);

  //-- create a bullet body
  btTransform pose = conv_raiTrans2btTrans(f->ensure_X());
  btScalar mass(1.0f);
  btVector3 localInertia(0, 0, 0);
  if(type==rai::BT_dynamic) {
    if(f->inertia) mass = f->inertia->mass;
    colShape->calculateLocalInertia(mass, localInertia);
  } else {
    mass=0.;
  }

  btDefaultMotionState* motionState = new btDefaultMotionState(pose);
  btRigidBody* body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, motionState, colShape, localInertia));

  //-- these are physics tweaks
  {
    double friction=opt.defaultFriction;
    for(auto s:shapes) if(s->frame.ats) s->frame.ats->get<double>(friction, "friction");
    if(friction>=0.) body->setFriction(friction);
  }
//  body->setRollingFriction(.01);
//  body->setSpinningFriction(.01);
  //cout <<body->getContactStiffness() <<' ' <<body->getContactDamping() <<endl;
  body->setContactStiffnessAndDamping(opt.contactStiffness, opt.contactDamping);
  {
    double restitution=opt.defaultRestitution;
    for(auto s:shapes) if(s->frame.ats) s->frame.ats->get<double>(restitution, "restitution");
    if(restitution>=0.) body->setRestitution(restitution);
  }

  dynamicsWorld->addRigidBody(body);

  if(type==rai::BT_kinematic) {
    body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  while(actors.N<=f->ID) actors.append(0);
  CHECK(!actors(f->ID), "you already added a frame with ID" <<f->ID);
  actors(f->ID) = body;
  return body;
}

btSoftBody* BulletInterface_self::addSoft(rai::Frame* f, int verbose) {
  //-- collect all shapes of that link
  CHECK_EQ(f->children.N, 0, "");
  //-- check inertia

  //-- decide on the type
  actorTypes(f->ID) = rai::BT_soft;
  if(verbose>0) LOG(0) <<"adding link anchored at '" <<f->name <<"' as " <<rai::Enum<rai::BodyType>(rai::BT_soft);

  //-- create a bullet collision shape
  rai::Mesh& m = f->shape->mesh();

  btSoftBody* softbody = btSoftBodyHelpers::CreateRope(softBodyWorldInfo,
                                                  conv_arr_btVec3(m.V[0]),
      conv_arr_btVec3(m.V[-1]),
      m.V.d0-2,
      1); //root fixed? tail fixed?
  softbody->m_cfg.piterations = 4;
  softbody->m_materials[0]->m_kLST = 0.5;
  softbody->setTotalMass(f->inertia->mass);
  auto world = dynamic_cast<btSoftRigidDynamicsWorld*>(dynamicsWorld);
  CHECK(world, "need a btSoftRigidDynamicsWorld");
  world->addSoftBody(softbody);

  while(actors.N<=f->ID) actors.append(0);
  CHECK(!actors(f->ID), "you already added a frame with ID" <<f->ID);
  actors(f->ID) = softbody;
  return softbody;
}

void BulletInterface::saveBulletFile(const char* filename) {
  //adapted from PhysicsServerCommandProcessor::processSaveBulletCommand

  FILE* f = fopen(filename, "wb");
  if(f) {
    btDefaultSerializer* ser = new btDefaultSerializer();
    int currentFlags = ser->getSerializationFlags();
    ser->setSerializationFlags(currentFlags); // | BT_SERIALIZE_CONTACT_MANIFOLDS);

    self->dynamicsWorld->serialize(ser);
    fwrite(ser->getBufferPointer(), ser->getCurrentBufferSize(), 1, f);
    fclose(f);
    delete ser;
  } else {
    HALT("could not open file '" <<filename <<"' for writing");
  }
}

btDiscreteDynamicsWorld*BulletInterface::getDynamicsWorld(){
  return self->dynamicsWorld;
}

btCollisionShape* BulletInterface_self::createCollisionShape(rai::Shape* s) {
  btCollisionShape* colShape=0;
  arr& size = s->size;
  switch(s->type()) {
    case rai::ST_sphere: {
      colShape = new btSphereShape(btScalar(s->radius()));
    } break;
    case rai::ST_box: {
      colShape = new btBoxShape(btVector3(.5*size(0), .5*size(1), .5*size(2)));
    } break;
//    case rai::ST_capsule: {
//      colShape = new btCapsuleShape(btScalar(s->radius()), btScalar(size(0)));
//    } break;
    case rai::ST_capsule:
    case rai::ST_cylinder:
    case rai::ST_ssCylinder:
    case rai::ST_ssBox:
    case rai::ST_ssCvx:
//    {
//#ifdef BT_USE_DOUBLE_PRECISION
//      arr& V = s->sscCore().V;
//#else
//      floatA V = convert<float>(s->sscCore().V);
//#endif
//      colShape = new btConvexHullShape(V.p, V.d0, V.sizeT*V.d1);
//      colShape->setMargin(s->radius());
//    } break;
    case rai::ST_mesh: {
#ifdef BT_USE_DOUBLE_PRECISION
      arr& V = s->mesh().V;
#else
      floatA V = convert<float>(s->mesh().V);
#endif
      colShape = new btConvexHullShape(V.p, V.d0, V.sizeT*V.d1);
      colShape->setMargin(0.);
    } break;
    default: HALT("NIY" <<s->type());
  }
  return colShape;
}

btCollisionShape* BulletInterface_self::createCompoundCollisionShape(rai::Frame* link, ShapeL& shapes) {
  btCompoundShape* colShape = new btCompoundShape;
  for(rai::Shape* s:shapes) {
    colShape->addChildShape(conv_raiTrans2btTrans(s->frame.ensure_X()/link->ensure_X()), createCollisionShape(s));
  }
  return colShape;
}

BulletBridge::BulletBridge(btDiscreteDynamicsWorld* _dynamicsWorld) : dynamicsWorld(_dynamicsWorld) {
  btCollisionObjectArray& collisionObjects = dynamicsWorld->getCollisionObjectArray();
  actors.resize(collisionObjects.size()).setZero();
  for(int i=0;i<collisionObjects.size();i++)  actors(i) = collisionObjects[i];
}

void BulletBridge::getConfiguration(rai::Configuration& C){
  btCollisionObjectArray& collisionObjects = dynamicsWorld->getCollisionObjectArray();
  for(int i=0;i<collisionObjects.size();i++){
    btCollisionObject* obj = collisionObjects[i];
    btCollisionShape* shape = obj->getCollisionShape();
    cout <<"OBJECT " <<i <<" type:" <<obj->getInternalType() <<" shapeType: " <<shape->getShapeType() <<' ' <<shape->getName() <<endl;
    if(obj->getInternalType()==obj->CO_COLLISION_OBJECT){
      rai::Transformation X;
      btTrans2raiTrans(X, obj->getWorldTransform());
      auto& f = C.addFrame(STRING("coll"<<i))
                ->setShape(rai::ST_marker, {.1})
                .setPose(X);
    }
    btRigidBody* body = dynamic_cast<btRigidBody*>(obj);
    if(body){
      rai::Transformation X;
      btTransform pose;
      if(body->getMotionState()) {
        body->getMotionState()->getWorldTransform(pose);
      } else {
        NIY; //trans = obj->getWorldTransform();
      }
      btTrans2raiTrans(X, pose);
      cout <<"OBJECT " <<i <<" pose: " <<X <<" shapeType: " <<shape->getShapeType() <<' ' <<shape->getName();
      switch(shape->getShapeType()){
        case CONVEX_HULL_SHAPE_PROXYTYPE:{
          btConvexHullShape* obj = dynamic_cast<btConvexHullShape*>(shape);
          arr V(obj->getNumPoints(), 3);
          for(uint i=0;i<V.d0;i++) V[i] = conv_btVec3_arr(obj->getUnscaledPoints()[i]);
          auto& f = C.addFrame(STRING("obj"<<i))
                    ->setConvexMesh(V)
                    .setPose(X);
          double mInv = body->getInvMass();
          if(mInv>0.) f.setMass(1./mInv);
        } break;
        case BOX_SHAPE_PROXYTYPE:{
          btBoxShape* box = dynamic_cast<btBoxShape*>(shape);
          arr size = 2.*conv_btVec3_arr(box->getHalfExtentsWithMargin());
          cout <<" margin: " <<box->getMargin() <<" size: " <<size;
          auto& f = C.addFrame(STRING("obj"<<i))
                    ->setShape(rai::ST_box, size)
                    .setPose(X);
          double mInv = body->getInvMass();
          if(mInv>0.) f.setMass(1./mInv);
        } break;
        case SPHERE_SHAPE_PROXYTYPE:{
          btSphereShape* obj = dynamic_cast<btSphereShape*>(shape);
          arr size = {obj->getRadius()};
          cout <<" margin: " <<obj->getMargin() <<" size: " <<size;
          auto& f = C.addFrame(STRING("obj"<<i))
                    ->setShape(rai::ST_sphere, size)
                    .setPose(X);
          double mInv = body->getInvMass();
          if(mInv>0.) f.setMass(1./mInv);
        } break;
        case CYLINDER_SHAPE_PROXYTYPE:{
          btCylinderShape* obj = dynamic_cast<btCylinderShape*>(shape);
          arr size = 2.*conv_btVec3_arr(obj->getHalfExtentsWithMargin());
          cout <<" margin: " <<obj->getMargin() <<" size: " <<size;
          size(1) = size(0);
          size(0) = size(2);
          size.resizeCopy(2);
          auto& f = C.addFrame(STRING("obj"<<i))
                    ->setShape(rai::ST_cylinder, size)
                    .setPose(X);
          double mInv = body->getInvMass();
          if(mInv>0.) f.setMass(1./mInv);

        } break;
        case COMPOUND_SHAPE_PROXYTYPE:
        case STATIC_PLANE_PROXYTYPE: {
          auto& f = C.addFrame(STRING("obj"<<i))
                    ->setShape(rai::ST_marker, {.1})
                    .setPose(X);
        } break;
        default:{
          NIY;
        }
      }
      cout <<endl;
    }
    btSoftBody* softbody = dynamic_cast<btSoftBody*>(obj);
    if(softbody){
      rai::Frame& f = C.addFrame(STRING("soft"<<i))
                      ->setShape(rai::ST_mesh, {});
      rai::Mesh &m = f.shape->mesh();
      {
        m.V.resize(softbody->m_nodes.size(), 3);
        for(int i=0; i<softbody->m_nodes.size(); i++){
          m.V[i] = conv_btVec3_arr(softbody->m_nodes[i].m_x);
        }
        m.makeLineStrip();
      }

      f.setMass(softbody->getTotalMass());
      f.inertia->type = rai::BT_soft;
    }
  }
  CHECK_EQ(C.frames.N, actors.N, "");
}



void BulletBridge::pullPoses(rai::Configuration& C, bool alsoStaticAndKinematic){
  CHECK_EQ(C.frames.N, actors.N, "");
  ::pullPoses(C.frames, actors, NoArr, alsoStaticAndKinematic);
}


#else

BulletInterface::BulletInterface(rai::Configuration& K, int verbose, bool yAxisGravity, bool enableSoftBodies) { NICO }
BulletInterface::~BulletInterface() { NICO }
void BulletInterface::step(double tau) { NICO }
void BulletInterface::pushFullState(const FrameL& frames, const arr& vel) { NICO }
void BulletInterface::pushKinematicStates(const FrameL& frames) { NICO }
void BulletInterface::pullDynamicStates(FrameL& frames, arr& vel) { NICO }
void BulletInterface::saveBulletFile(const char* filename) { NICO }
void BulletInterface::changeObjectType(rai::Frame* f, int _type, const arr& withVelocity) { NICO }

#endif

#ifdef RAI_BULLET
RUN_ON_INIT_BEGIN(kin_bullet)
rai::Array<btCollisionObject*>::memMove=true;
rai::Array<rai::BodyType>::memMove=true;
RUN_ON_INIT_END(kin_bullet)
#endif
