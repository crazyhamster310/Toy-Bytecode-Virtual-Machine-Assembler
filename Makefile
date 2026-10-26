.PHONY: help check clean
.PHONY: setup-asm test-asm lint-asm format-asm check-asm
.PHONY: build-vm test-vm lint-vm format-vm check-vm

ASM_DIR   := assembler
VENV_DIR  := $(CURDIR)/$(ASM_DIR)/.venv
PYTHON    := $(VENV_DIR)/bin/python
PIP       := $(VENV_DIR)/bin/pip
PYTEST    := $(VENV_DIR)/bin/pytest
RUFF      := $(VENV_DIR)/bin/ruff
MYPY      := $(VENV_DIR)/bin/mypy

all: check

help: ## Показать список доступных команд
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-16s\033[0m %s\n", $$1, $$2}'

check: check-asm check-vm ## Полная проверка всех компонентов проекта

clean: ## Очистка рабочих файлов
	rm -rf build/
	find . -type f -name "*.bin" -delete

setup-asm: ## Создать .venv и установить ассемблер в dev-режиме
	python3 -m venv $(VENV_DIR)
	$(PIP) install --upgrade pip
	$(PIP) install -e "$(ASM_DIR)[dev]"

$(VENV_DIR):
	@echo "Ошибка: виртуальное окружение не найдено. Выполните 'make setup-asm'."
	@exit 1

test-asm: | $(VENV_DIR) ## Запуск тестов ассемблера
	cd assembler && ${PYTEST} -v

lint-asm: | $(VENV_DIR) ## Запуск линтера и проверки типов
	cd assembler && ${RUFF} check .
	cd assembler && ${MYPY} src tests

format-asm: | $(VENV_DIR) ## Автоформатирование кода ассемблера
	cd assembler && ${RUFF} format .

check-asm: lint-asm test-asm ## Полная проверка ассемблера

build-vm: ## Сборка виртуальной машины
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug && cmake --build build

test-vm: build-vm ## Запуск тестов виртуальной машины
	ctest --test-dir build --output-on-failure

format-vm: ## Автоформатирование кода виртуальной машины
	find runtime -name '*.hpp' -o -name '*.cpp' | xargs clang-format -i

lint-vm: build-vm ## Запуск статического анализатора
	clang-tidy -p build runtime/src/*.cpp runtime/tests/*.cpp

check-vm: lint-vm test-vm ## Полная проверка виртуальной машины