DOC_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_SCRIPT := $(DOC_ROOT)scripts/build-doc.sh
PUBLISH_PDF := $(DOC_ROOT)publish/cuwacunu-algorithm.pdf
FAST_PDF := $(DOC_ROOT)publish/cuwacunu-algorithm-fast.pdf

.DEFAULT_GOAL := publish

.PHONY: all publish pdf fast check clean distclean help

all: publish

publish pdf:
	@printf "[DOC] checking formal algorithm PDF\n"
	@printf "[DOC] target : %s\n" "$(PUBLISH_PDF)"
	+$(MAKE) --no-print-directory $(PUBLISH_PDF)
	@if [ -s "$(PUBLISH_PDF)" ]; then \
	  bytes=$$(wc -c < "$(PUBLISH_PDF)"); \
	  printf "[DOC] ready  : %s (%s bytes)\n" "$(PUBLISH_PDF)" "$$bytes"; \
	else \
	  printf "[DOC] missing: %s\n" "$(PUBLISH_PDF)" >&2; \
	  exit 1; \
	fi

$(PUBLISH_PDF): $(DOC_ROOT)algorithm.tex $(wildcard $(DOC_ROOT)sections/*.tex) $(BUILD_SCRIPT)
	@printf "[DOC] rebuilding publish artifact\n"
	@printf "[DOC] main   : %s\n" "$(DOC_ROOT)algorithm.tex"
	@printf "[DOC] script : %s\n" "$(BUILD_SCRIPT)"
	$(BUILD_SCRIPT)

fast:
	@printf "[DOC] checking fast draft PDF\n"
	@printf "[DOC] target : %s\n" "$(FAST_PDF)"
	+$(MAKE) --no-print-directory $(FAST_PDF)
	@if [ -s "$(FAST_PDF)" ]; then \
	  bytes=$$(wc -c < "$(FAST_PDF)"); \
	  printf "[DOC] ready  : %s (%s bytes)\n" "$(FAST_PDF)" "$$bytes"; \
	else \
	  printf "[DOC] missing: %s\n" "$(FAST_PDF)" >&2; \
	  exit 1; \
	fi

$(FAST_PDF): $(DOC_ROOT)algorithm.tex $(wildcard $(DOC_ROOT)sections/*.tex) $(BUILD_SCRIPT)
	@printf "[DOC] rebuilding fast draft artifact\n"
	@printf "[DOC] main   : %s\n" "$(DOC_ROOT)algorithm.tex"
	@printf "[DOC] script : %s\n" "$(BUILD_SCRIPT)"
	LATEX_PASSES=1 DOC_BASENAME=cuwacunu-algorithm-fast $(BUILD_SCRIPT)

check:
	@printf "[DOC] checking build wrapper syntax\n"
	bash -n $(BUILD_SCRIPT)
	@printf "[DOC] checking main source exists\n"
	test -f $(DOC_ROOT)algorithm.tex

clean:
	@printf "[DOC] removing %s\n" "$(DOC_ROOT)build"
	rm -rf $(DOC_ROOT)build

distclean: clean
	@printf "[DOC] removing %s\n" "$(DOC_ROOT)publish"
	rm -rf $(DOC_ROOT)publish

help:
	@echo "doc targets:"
	@echo "  publish   build doc/publish/cuwacunu-algorithm.pdf"
	@echo "  pdf       alias for publish"
	@echo "  fast      build doc/publish/cuwacunu-algorithm-fast.pdf with one LaTeX pass"
	@echo "  check     syntax-check the secure build wrapper"
	@echo "  clean     remove doc/build"
	@echo "  distclean remove doc/build and doc/publish"
