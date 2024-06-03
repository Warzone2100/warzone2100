import fileinput
import re

lines_seen = dict() # holds lines already seen (minus trailing comments), mapped to a list of source references
output_lines = list()

# See: https://stackoverflow.com/a/241506
def comment_remover(text):
	def replacer(match):
		s = match.group(0)
		if s.startswith('/'):
			return " " # note: a space and not an empty string
		return s
	pattern = re.compile(
		r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
		re.DOTALL | re.MULTILINE
	)
	return re.sub(pattern, replacer, text)

def src_ref_comment_extractor(text, remove_prefix="// SRC:"):
	pattern = re.compile(
		r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
		re.DOTALL | re.MULTILINE
	)
	comments = list()
	for m in re.finditer(pattern, text):
		if m.group(0).startswith("//"):
			# found a comment
			s = m.group(0).rstrip('\n')
			s = s[len(remove_prefix):] if s.startswith(remove_prefix) else s
			comments.append(s.strip())
	return comments

with fileinput.input() as f_input:
	for line in f_input:
		line = line.rstrip('\n')
		line_without_comments = comment_remover(line)
		if line_without_comments not in lines_seen: # not a duplicate
			lines_seen[line_without_comments] = src_ref_comment_extractor(line)
			output_lines.append(line_without_comments)
		else:
			# store the additional src refs for the line
			lines_seen[line_without_comments].extend(src_ref_comment_extractor(line))

MAX_COMMENT_LINES = 4
for line in sorted(output_lines, key=str.casefold):
	src_ref_limit = MAX_COMMENT_LINES
	sorted_src_refs = sorted(lines_seen[line], key=str.casefold)
	if len(sorted_src_refs) > src_ref_limit:
		src_ref_limit -= 1
	for idx, src_ref in zip(range(src_ref_limit), sorted_src_refs):
		if idx == 0:
			print("// TRANSLATORS: ")
		print("// " + src_ref)
	if len(sorted_src_refs) > src_ref_limit:
		print("// ... + " + str(len(sorted_src_refs) - src_ref_limit) + " refs")
	if "%" in line:
		# ensure xgettext doesn't decide to treat this as a c-format line
		print("// xgettext:no-c-format")
	print(line)
