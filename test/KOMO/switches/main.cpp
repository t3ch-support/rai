#include <KOMO/komo.h>
#include <string>
#include <map>
#include <Core/graph.h>

using namespace std;

//===========================================================================

void TEST(Grasp){
  mlr::KinematicWorld K("model.g");
  K.optimizeTree(false);
  K.checkConsistency();
  FILE("z.g") <<K;

  KOMO komo;
  komo.setModel(K);
  komo.setPathOpt(2.5, 10., 5.);
//  komo.setIKOpt();

//  komo.setPosition(1., -1., "endeff", "stick");
//  komo.setTouch(1., -1., "endeff", "stick");

  komo.setGrasp(1., "endeff", "stick");
  komo.setTouch(2., -1., "stick", "redBall");
  komo.setTouch(-1.,-1., "stick", "table1", OT_ineq, {}, 1e2);
  komo.setTouch(-1.,-1., "stick", "arm1", OT_ineq, {}, 1e2);

  komo.setSlow(2., -1.,1e0);

  komo.reset();
  komo.checkGradients();
  komo.run();
  komo.checkGradients();

  Graph result = komo.getReport(true);

  for(uint i=0;i<2;i++) if(!komo.displayTrajectory(.1, true)) break;
}

//===========================================================================

int main(int argc,char** argv){
  mlr::initCmdLine(argc,argv);

  testGrasp();

  return 0;
}

