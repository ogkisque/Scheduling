# Scheduling

Реализация алгоритма планирования инструкций на основе времен раннего и позднего.

## Сборка и запуск
```
cmake -S . -B build
cmake --build build
./build/src/main test.txt
```

## Запуск тестов
```
cd tests
python3 check.py ./../build/src/main 3
```