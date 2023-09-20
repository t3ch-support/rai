#pragma once

namespace rai {

enum SkeletonSymbol {
  SY_none=-1,

  //geometric:
  SY_touch, SY_above, SY_inside, SY_oppose,

  //pose constraints:
  SY_poseEq, SY_stableRelPose, SY_stablePose,

  //mode switches:
  SY_stable, SY_stableOn, SY_dynamic, SY_dynamicOn, SY_dynamicTrans, SY_quasiStatic, SY_quasiStaticOn, SY_downUp, SY_break, SY_stableZero,

  //interactions:
  SY_contact, SY_contactStick, SY_contactComplementary, SY_bounce, SY_push,

  //mode switches:
  SY_magic, SY_magicTrans,

  //grasps/placements:
  SY_topBoxGrasp, SY_topBoxPlace,

  SY_dampMotion,

  SY_identical,

  SY_alignByInt,

  SY_makeFree, SY_forceBalance,

  SY_relPosY,

  SY_touchBoxNormalX, SY_touchBoxNormalY, SY_touchBoxNormalZ,

  SY_boxGraspX, SY_boxGraspY, SY_boxGraspZ,

  SY_stableYPhi,

  SY_end,

  SY_pointPos,

  SY_alignXZ,
  
  SY_loopTree
};

} //namespace
