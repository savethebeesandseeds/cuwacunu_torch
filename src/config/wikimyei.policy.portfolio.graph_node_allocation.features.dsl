/*
  wikimyei.policy.portfolio.graph_node_allocation.features.dsl
  ==================================================================
  Actor-visible feature manifest for the graph-node allocation policy.

  Identity fields such as anchor keys, timestamps, schema ids, digests,
  graph fingerprints, causal schedule digest, and snapshot family digest stay
  evidence-only. The actor sees only the named tensors below.
*/
GRAPH_NODE_ALLOCATION_FEATURES {
  VERSION = wikimyei.policy.portfolio.graph_node_allocation.features.v1;
  POLICY_INPUT_SCHEMA = kikijyeba.environment.policy_input.v1;
  NODE_FEATURE_NAMES = current_weight,previous_target_weight,weight_delta_from_previous_target,log_executable_mid,valid_mask,tradable_mask,executable_mask,expected_log_return,expected_arithmetic_return,marginal_variance,marginal_volatility,var_down,cvar_down,adverse_excursion_probability,volatility,confidence,liquidity_score,linear_cost,quadratic_impact,capacity_weight_limit,projection_validation_score,residual_quality_score,surprise,calibration_score,mixture_entropy,component_disagreement,channel_disagreement,is_accounting_numeraire_node;
  NODE_FEATURE_TRANSFORMS = identity,identity,identity,log_positive,mask_bool,mask_bool,mask_bool,identity,identity,nonnegative,nonnegative,identity,identity,probability,nonnegative,unit_interval,unit_interval,nonnegative,nonnegative,unit_interval,unit_interval,unit_interval,nonnegative,unit_interval,nonnegative,nonnegative,nonnegative,mask_bool;
  NODE_FEATURE_DEFAULT_POLICY = neutral_allocation_belief_fallbacks.v1;
  NODE_FEATURE_CLIPPING_POLICY = producer_validated_no_actor_clip.v1;
  GLOBAL_FEATURE_NAMES = log_equity_value_numeraire,drawdown,current_weight_sum,previous_target_weight_sum,current_accounting_numeraire_weight,previous_accounting_numeraire_weight;
  GLOBAL_FEATURE_TRANSFORMS = log_positive,nonnegative,identity,identity,unit_interval,unit_interval;
  GLOBAL_FEATURE_DEFAULT_POLICY = portfolio_state_required.v1;
  GLOBAL_FEATURE_CLIPPING_POLICY = producer_validated_no_actor_clip.v1;
  RISK_FEATURE_BLOCK = compact_cross_node_risk_block.v1;
  RISK_FEATURE_NAMES = mean_pairwise_correlation,max_pairwise_correlation,top1_correlation_eigenvalue,top2_correlation_eigenvalue,top3_correlation_eigenvalue,top1_covariance_eigenvalue,top2_covariance_eigenvalue,top3_covariance_eigenvalue,current_portfolio_volatility_estimate,current_portfolio_cvar_estimate;
  RISK_FEATURE_TRANSFORMS = bounded_correlation,bounded_correlation,nonnegative,nonnegative,nonnegative,nonnegative,nonnegative,nonnegative,nonnegative,identity;
  RISK_FEATURE_DEFAULT_POLICY = zero_if_no_allocation_belief.v1;
  RISK_FEATURE_CLIPPING_POLICY = producer_validated_no_actor_clip.v1;
  EVIDENCE_ONLY_FIELDS = schema_id,environment_assembly_id,observation_anchor_key,observation_anchor_index,knowledge_timestamp_ms,graph_order_fingerprint,allocation_belief_digest,execution_profile_digest,reward_contract_id,accounting_numeraire_node_id,causal_schedule_digest,snapshot_family_digest,policy_input_digest,policy_artifact_digest,policy_net_digest,policy_dsl_digest,policy_jkimyei_digest;
  RAW_MDN_INPUT_ALLOWED = false;
  FULL_SCENARIO_BANK_INPUT_ALLOWED = false;
};
