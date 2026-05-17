/*
  jkimyei.representation.vicreg.dsl
  ==========================================
  Default smoke training policy for graph-first VICReg representation.
*/
TRAINING {
  VERSION = jkimyei.representation.vicreg.v1;
  TRAINING_ID = smoke_vicreg_node_representation;
  TASK = vicreg_node_representation;
  COMPONENT_ID = node_vicreg_v1;
  OPTIMIZER = adam;
  LEARNING_RATE = 0.001;
  MAX_STEPS = 1;
  BATCH_SIZE = 2;
  GRAD_CLIP_NORM = 0.0;
  CHECKPOINT_EVERY = 0;
  REPORT_EVERY = 1;
  VALIDATION_EVERY = 0;
  SEED = 17;
  FREEZE_REPRESENTATION = false;
};
