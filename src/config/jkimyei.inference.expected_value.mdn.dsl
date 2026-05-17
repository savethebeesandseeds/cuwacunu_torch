/*
  jkimyei.inference.expected_value.mdn.dsl
  =================================================
  Default one-step smoke training policy for mdn ExpectedValue.
*/
TRAINING {
  VERSION = jkimyei.inference.expected_value.mdn.v1;
  TRAINING_ID = smoke_mdn_inference;
  TASK = mdn_expected_value_inference;
  COMPONENT_ID = mdn_v1;
  OPTIMIZER = adam;
  LEARNING_RATE = 0.01;
  MAX_STEPS = 1;
  BATCH_SIZE = 2;
  GRAD_CLIP_NORM = 0.0;
  CHECKPOINT_EVERY = 0;
  REPORT_EVERY = 1;
  VALIDATION_EVERY = 0;
  SEED = 31;
  FREEZE_REPRESENTATION = true;
};
