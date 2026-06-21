/* ujcamei/source/contract/syntax/tokens.h */
#pragma once

#include "piaabo/core/utils.h"

#undef SOURCE_PIPELINE_DEBUG /* define to see verbose parsing output */

DEFINE_HASH(SOURCE_PIPELINE_HASH_instruction, "<instruction>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_instrument_table, "<instrument_table>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_table, "<channel_table>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_node_table, "<graph_node_table>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_edge_table, "<graph_edge_table>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_policy_block, "<graph_policy_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_edge_resolution_policy_assignment,
            "<edge_resolution_policy_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_edge_source_kind_assignment,
            "<edge_source_kind_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_fetch_mode_assignment,
            "<fetch_mode_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_max_fetch_workers_assignment,
            "<max_fetch_workers_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_parallel_min_work_items_assignment,
            "<parallel_min_work_items_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_block, "<channel_set_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_active_assignment,
            "<channel_set_active_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_record_type_assignment,
            "<channel_set_record_type_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_input_length_assignment,
            "<channel_set_input_length_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_future_length_assignment,
            "<channel_set_future_length_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_weight_assignment,
            "<channel_set_weight_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_normalization_assignment,
            "<channel_set_normalization_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_set_intervals_assignment,
            "<channel_set_intervals_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_instrument_header_line,
            "<instrument_header_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_instrument_form, "<instrument_form>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_header_line, "<channel_header_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_form, "<channel_form>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_node_header_line,
            "<graph_node_header_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_node_form, "<graph_node_form>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_edge_header_line,
            "<graph_edge_header_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_graph_edge_form, "<graph_edge_form>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_table_top_line, "<table_top_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_table_divider_line, "<table_divider_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_table_bottom_line, "<table_bottom_line>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_comment, "<comment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_csv_policy_block, "<csv_policy_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_csv_bootstrap_assignment,
            "<csv_bootstrap_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_csv_step_abs_tol_assignment,
            "<csv_step_abs_tol_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_csv_step_rel_tol_assignment,
            "<csv_step_rel_tol_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_data_analytics_policy_block,
            "<data_analytics_policy_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_data_analytics_max_samples_assignment,
            "<data_analytics_max_samples_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_data_analytics_max_features_assignment,
            "<data_analytics_max_features_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_data_analytics_mask_epsilon_assignment,
            "<data_analytics_mask_epsilon_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_data_analytics_standardize_epsilon_assignment,
            "<data_analytics_standardize_epsilon_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_source_defaults_block,
            "<source_defaults_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_default_source_root_assignment,
            "<default_source_root_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_default_kline_intervals_assignment,
            "<default_kline_intervals_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_set_block,
            "<kline_source_set_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_instrument_assignment,
            "<kline_source_instrument_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_market_type_assignment,
            "<kline_source_market_type_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_venue_assignment,
            "<kline_source_venue_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_base_asset_assignment,
            "<kline_source_base_asset_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_quote_asset_assignment,
            "<kline_source_quote_asset_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_kind_assignment,
            "<kline_source_kind_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_source_root_assignment,
            "<kline_source_root_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_intervals_assignment,
            "<kline_intervals_assignment>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_kline_interval_list, "<kline_interval_list>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_source_root_path, "<source_root_path>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_market_type, "<market_type>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_venue, "<venue>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_base_asset, "<base_asset>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_quote_asset, "<quote_asset>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_source_kind, "<source_kind>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_node_id, "<node_id>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_node_kind, "<node_kind>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_edge_id, "<edge_id>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_base_node, "<base_node>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_quote_node, "<quote_node>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_source_instrument, "<source_instrument>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_edge_resolution_policy,
            "<edge_resolution_policy>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_edge_source_kind, "<edge_source_kind>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_fetch_mode, "<fetch_mode>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_policy_unsigned_int, "<policy_unsigned_int>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_policy_float, "<policy_float>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_normalization_policy,
            "<normalization_policy>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_source, "<source>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_break_block, "<break_block>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_file_path, "<file_path>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_active, "<active>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_input_length, "<input_length>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_future_length, "<future_length>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_channel_weight, "<channel_weight>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_character, "<character>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_literal, "<literal>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_whitespace, "<whitespace>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_instrument, "<instrument>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_record_type, "<record_type>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_interval, "<interval>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_boolean, "<boolean>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_special, "<special>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_letter, "<letter>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_number, "<number>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_newline, "<newline>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_empty, "<empty>");
DEFINE_HASH(SOURCE_PIPELINE_HASH_frame_char, "<frame_char>");
