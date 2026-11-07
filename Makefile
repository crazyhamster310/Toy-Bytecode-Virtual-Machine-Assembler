.PHONY: help check clean test-e2e
.PHONY: setup-asm test-asm lint-asm format-asm check-asm build-asm-dist
.PHONY: build-vm build-vm-release test-vm lint-vm format-vm check-vm

ASM_DIR   := assembler
VENV_DIR  := $(CURDIR)/$(ASM_DIR)/.venv
PYTHON    := $(VENV_DIR)/bin/python
PIP       := $(VENV_DIR)/bin/pip
PYTEST    := $(VENV_DIR)/bin/pytest
RUFF      := $(VENV_DIR)/bin/ruff
MYPY      := $(VENV_DIR)/bin/mypy

BUILD_DIR_DEBUG   ?= build/debug
BUILD_DIR_RELEASE ?= build/release

all: check

help: ## Display this help message
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-16s\033[0m %s\n", $$1, $$2}'

check: check-asm check-vm test-e2e ## Run all checks across assembler and virtual machine

clean: ## Remove build artifacts and compiled bytecode files
	rm -rf build/
	find . -type f -name "*.bin" -delete

setup-asm: ## Create virtualenv and install assembler in dev-mode
	python3 -m venv $(VENV_DIR)
	$(PIP) install --upgrade pip
	$(PIP) install -e "$(ASM_DIR)[dev]"
	@echo "==> Done. Activate with: source $(ASM_DIR)/.venv/bin/activate"

$(VENV_DIR):
	@echo "error: virtual environment not found. Run 'make setup-asm' first."
	@exit 1

test-asm: | $(VENV_DIR) ## Run assembler tests
	cd assembler && ${PYTEST} -v

lint-asm: | $(VENV_DIR) ## Run assembler linters and type checkers
	cd assembler && ${RUFF} check .
	cd assembler && ${MYPY} src tests

format-asm: | $(VENV_DIR) ## Format assembler source code
	cd assembler && ${RUFF} format .

check-asm: lint-asm test-asm ## Run all assembler verifications

build-asm-dist: | $(VENV_DIR) ## Build distribution wheel and tarball for assembler
	$(PIP) install build
	cd $(ASM_DIR) && $(PYTHON) -m build

build-vm: ## Build VM runtime in Debug mode
	cmake -B $(BUILD_DIR_DEBUG) -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON -DBUILD_TESTING=ON
	cmake --build $(BUILD_DIR_DEBUG)

build-vm-release: ## Build VM runtime in Release mode
	cmake -B $(BUILD_DIR_RELEASE) -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF -DBUILD_TESTING=OFF
	cmake --build $(BUILD_DIR_RELEASE) --target byte-vm byte-disasm

test-vm: build-vm ## Run virtual machine tests
	ctest --test-dir $(BUILD_DIR_DEBUG) --output-on-failure

format-vm: ## Format virtual machine source code
	find runtime -name '*.hpp' -o -name '*.cpp' | xargs clang-format -i

lint-vm: build-vm ## Run static analyzer on virtual machine code
	clang-tidy -p $(BUILD_DIR_DEBUG) --header-filter='runtime/.*' runtime/src/*.cpp runtime/tests/*.cpp

check-vm: lint-vm test-vm ## Run all VM verifications

test-e2e: build-vm | $(VENV_DIR) ## Run end-to-end integration test
	@echo "==> Running E2E: Factorial"
	$(PYTHON) -m byte_asm.cli examples/factorial.asm -o examples/factorial.bin
	@FACT_OUT=$$(echo "5" | $(BUILD_DIR_DEBUG)/byte-vm examples/factorial.bin) && \
	if [ "$$FACT_OUT" = "120" ]; then \
		echo "    [PASS] factorial(5) = $$FACT_OUT"; \
	else \
		echo "    [FAIL] expected 120, got '$$FACT_OUT'" && exit 1; \
	fi
	@echo "==> Running E2E: Fibonacci"
	$(PYTHON) -m byte_asm.cli examples/fibonacci.asm -o examples/fibonacci.bin
	@FIB_OUT=$$(echo "10" | $(BUILD_DIR_DEBUG)/byte-vm examples/fibonacci.bin) && \
	if [ "$$FIB_OUT" = "55" ]; then \
		echo "    [PASS] fibonacci(10) = $$FIB_OUT"; \
	else \
		echo "    [FAIL] expected 55, got '$$FIB_OUT'" && exit 1; \
	fi
	@echo "==> All E2E tests PASSED successfully!"