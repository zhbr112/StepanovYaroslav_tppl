import sys
from collections import Counter

try:
    filepath = sys.argv[1]
except IndexError:
    filepath = input("Введите имя файла: ")

try:
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()
except FileNotFoundError:
    print(f"Ошибка: Файл '{filepath}' не найден.")
    sys.exit()

content = "".join(lines)
total_lines = len(lines)
total_chars = len(content)
empty_lines = sum(1 for line in lines if not line.strip())
char_frequency = Counter(content)

print("\nЧто вывести? (введите номера через пробел, например: 1 3)")
print("  1: Кол-во строк")
print("  2: Кол-во символов")
print("  3: Кол-во пустых строк")
print("  4: Частота символов")
choices = input("Ваш выбор: ").split()

print("\n--- Результаты анализа ---")

if '1' in choices:
    print(f"Количество строк: {total_lines}")

if '2' in choices:
    print(f"Количество символов: {total_chars}")

if '3' in choices:
    print(f"Количество пустых строк: {empty_lines}")

if '4' in choices:
    print("Частотный словарь символов:")
    for char, count in char_frequency.most_common():
        char_repr = {'\n': r"'\n'", ' ': "' '", '\t': r"'\t'"}.get(char, f"'{char}'")
        print(f"  {char_repr}: {count}")

if not choices:
    print("Ничего не выбрано.")