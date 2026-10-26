.PHONY: help setup-asm test-asm lint-asm format-asm check-asm check build-vm clean

ASM_DIR   := assembler
VENV_DIR  := $(CURDIR)/$(ASM_DIR)/.venv
PYTHON    := $(VENV_DIR)/bin/python
PIP       := $(VENV_DIR)/bin/pip
PYTEST    := $(VENV_DIR)/bin/pytest
RUFF      := $(VENV_DIR)/bin/ruff
MYPY      := $(VENV_DIR)/bin/mypy

all: check build-vm

help: ## Показать список доступных команд
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-16s\033[0m %s\n", $$1, $$2}'

setup-asm: ## Создать .venv и установить ассемблер в dev-режиме
	python3 -m venv $(VENV_DIR)
	$(PIP) install --upgrade pip
	$(PIP) install -e "$(ASM_DIR)[dev]"

$(VENV_DIR):
	@echo "Ошибка: виртуальное окружение не найдено. Выполните 'make setup-asm'."
	@exit 1

test-asm: | $(VENV_DIR) ## Запуск тестов ассемблера (pytest)
	cd assembler && ${PYTEST} -v

lint-asm: | $(VENV_DIR) ## Запуск линтера (ruff) и проверки типов (mypy)
	cd assembler && ${RUFF} check .
	cd assembler && ${MYPY} src tests

format-asm: | $(VENV_DIR) ## Автоформатирование кода ассемблера
	cd assembler && ${RUFF} format .

check-asm: lint-asm test-asm ## Полная проверка ассемблера (линтер, типы, тесты)

check: check-asm ## Полная проверка всех компонентов проекта

build-vm: ## Сборка виртуальной машины
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug && cmake --build build

clean: ## Очистка рабочих файлов
	rm -rf build/
	find . -type f -name "*.bin" -delete