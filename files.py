import os

for filename in os.listdir('.'):
    if os.path.isfile(filename):
        print(f"---{filename}---")
        try:
            with open(filename, 'r', encoding='utf-8') as f:
                print(f.read())
        except UnicodeDecodeError:
            print("[binary file, skipping]")
        print()
