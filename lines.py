import os

def count_lines_in_file(file_path):
    with open(file_path, 'r') as file:
        return sum(1 for line in file if not line.strip().startswith(('/', '*')))


def count_lines_in_file(file_path):
    with open(file_path, 'r') as file:
        return sum(1 for _ in file)


def count_lines_in_folder(src_folder):
    line_counts = {}
    for element in os.listdir(src_folder):
        element_path = os.path.join(src_folder, element)
        if os.path.isfile(element_path):
            if (element_path.endswith('.cpp') or element_path.endswith('.hpp')
                    or element_path.endswith('.h') or element_path.endswith('.c')
                    or element_path.endswith('.ipp')):
                line_counts[element_path] = count_lines_in_file(element_path)
        elif os.path.isdir(element_path):
            if element_path.endswith('kissat'): continue
            subfolder_line_counts = count_lines_in_folder(element_path)
            line_counts.update(subfolder_line_counts)
    return line_counts

if __name__ == "__main__":
    line_counts_src = count_lines_in_folder('src')
    line_counts_include = count_lines_in_folder('include')
    total_lines = sum(line_counts_src.values()) + sum(line_counts_include.values())
    line_counts = {**line_counts_src, **line_counts_include}
    for file_path, line_count in line_counts.items():
        print(f"{file_path}: {line_count} lines")
    print(f"Total: {total_lines} lines")
