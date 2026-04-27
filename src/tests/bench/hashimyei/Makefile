# =============================================================================
# tests/bench/hashimyei/Makefile — hashimyei focused tests
# =============================================================================
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/hashimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

DB_LINK   = $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)
DATA_LINK = $(libtorch_a) $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)

$(eval $(call TEST_ONEFILE, test_hashimyei_catalog, test_hashimyei_catalog.cpp, $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_hashimyei_report_fragments, test_hashimyei_report_fragments.cpp, $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_lattice_catalog, test_lattice_catalog.cpp, $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_source_runtime_projection, test_source_runtime_projection.cpp))
$(eval $(call TEST_ONEFILE, test_hero_mcp_surface, test_hero_mcp_surface.cpp))

$(TEST_OUT)/test_hashimyei_catalog: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_hashimyei_report_fragments: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_lattice_catalog: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)

HERO_MCP_SURFACE_PARTS := \
  $(HERE_PATH)/test_hero_mcp_surface_private_harness.cpp \
  $(HERE_PATH)/test_hero_mcp_surface_private_catalog.cpp \
  $(HERE_PATH)/test_hero_mcp_surface_private_marshal_flow.cpp \
  $(HERE_PATH)/test_hero_mcp_surface_private_marshal_edges.cpp

$(TEST_OUT)/test_hero_mcp_surface: $(HERO_MCP_SURFACE_PARTS)

.PHONY: hero-mcp-binaries
hero-mcp-binaries: $(HERO_OUT)/hero_hashimyei.mcp $(HERO_OUT)/hero_lattice.mcp $(HERO_OUT)/hero_marshal.mcp $(HERO_OUT)/hero_human.mcp

$(HERO_OUT)/hero_hashimyei.mcp:
	+$(MAKE) -C $(ROOT_PATH)/main/hero $(HERO_OUT)/hero_hashimyei.mcp

$(HERO_OUT)/hero_lattice.mcp:
	+$(MAKE) -C $(ROOT_PATH)/main/hero $(HERO_OUT)/hero_lattice.mcp

$(HERO_OUT)/hero_marshal.mcp:
	+$(MAKE) -C $(ROOT_PATH)/main/hero $(HERO_OUT)/hero_marshal.mcp

$(HERO_OUT)/hero_human.mcp:
	+$(MAKE) -C $(ROOT_PATH)/main/hero $(HERO_OUT)/hero_human.mcp

$(TEST_OUT)/test_hero_mcp_surface: $(HERO_OUT)/hero_hashimyei.mcp $(HERO_OUT)/hero_lattice.mcp $(HERO_OUT)/hero_marshal.mcp $(HERO_OUT)/hero_human.mcp

.PHONY: all
all: \
	$(TEST_OUT)/test_hashimyei_catalog \
	$(TEST_OUT)/test_hashimyei_report_fragments \
	$(TEST_OUT)/test_lattice_catalog \
	$(TEST_OUT)/test_source_runtime_projection \
	$(TEST_OUT)/test_hero_mcp_surface
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_hashimyei_catalog run-test_hashimyei_report_fragments run-test_lattice_catalog run-test_source_runtime_projection run-test_hero_mcp_surface

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) \
	  $(TEST_OUT)/test_hashimyei_catalog \
	  $(TEST_OUT)/test_hashimyei_report_fragments \
	  $(TEST_OUT)/test_lattice_catalog \
	  $(TEST_OUT)/test_source_runtime_projection \
	  $(TEST_OUT)/test_hero_mcp_surface
