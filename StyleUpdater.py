import re
import argparse

titles_source = {
    'GENERAL INCLUDES'  : 'Includes',
    'LITERAL CONSTANTS' : 'Preprocessor',
    'MEMORY CONSTANTS'  : 'Globals',
    'MACROS'            : None,
    'TYPE DEFINITIONS'  : 'Types',
    'VARIABLES'         : None,
    'PROCEDURES'        : 'Methods',
}

titles_header = {
    'GENERAL INCLUDES'  : 'Includes',
    'LITERAL CONSTANTS' : 'Preprocessor',
    'MEMORY CONSTANTS'  : 'Globals',
    'MACROS'            : None,
    'TYPE DEFINITIONS'  : 'Types',
    'CLASSES'           : 'Class Definition',
    'VARIABLES'         : 'Members',
    'PROCEDURES'        : 'Methods',
}

MAX_NUM_DASH = 82
TITLE_BREAK = '//' + ('-' * MAX_NUM_DASH) + '\n'
FUNCTION_BREAK = '/**' + ('-' * (MAX_NUM_DASH - 21)) + '\n'
SSN = '//\n'
STAR = '*\n'


parser = argparse.ArgumentParser(description='Updates the style of a file')
parser.add_argument('--file', '-f', help='File to update, .cpp or .h for specific files, leaving extension off will update both')
args = parser.parse_args()
input_files = args.file

files = []
if '.cpp' in input_files:
    files.append((input_files, True, titles_source))
elif '.h' in input_files:
    files.append((input_files, False, titles_header))
else:
    files.append((input_files + '.cpp', True, titles_source))
    files.append((input_files + '.h',   False, titles_header))

full_start_p = re.compile('/\*(\*)*')
full_end_p = re.compile('\*(\*)*/')
copyright_p = re.compile('(?P<start>20[0-9]{2})(-20[0-9]{2})*[\S\s]*')
function_p = re.compile('\s*(?P<return>[\w\*]*)\s*(?P<class>[\w\*]*)::(?P<function>\w*)\s*\(\s*(?P<params>[\w\s\n,&:\<\>\*]*)\s*\)')
closing_bracket_p = re.compile('}\s*(//|/\*)\s*(?P<function>[\w]*)')
whitespace_p = re.compile('^\s+')
todo_p = re.compile('(?P<start>.*)(todo|TODO|Todo){1}(?P<end>.*)')
member_p = re.compile('(?P<member>[\w=\.\*,\s]+);\s*(?P<comment>//[/\w=\.\*,\s]+)?')
ua_p = re.compile('\s*(U|A)')

for input_file in files:
    filename = input_file[0]
    source = input_file[1]
    titles = input_file[2]

    if filename is not None:
        with open(filename, 'r') as file:
            buffer = file.readlines()

        new_filename = '_' + filename
        with open(new_filename, 'w') as file:
            buffer.reverse()
            while buffer:
                line = buffer.pop()

                # Comment Blocks
                if re.match(full_start_p, line):
                    # Get comment text
                    comments = []
                    while True:
                        line = buffer.pop()
                        if re.match(full_end_p, line):
                            break
                        else:
                            comments.append(line)

                    # Title block
                    for comment in comments:
                        for title in titles:
                            if title in comment:
                                if titles[title] is not None:
                                    file.write(TITLE_BREAK)
                                    file.write('// ' + titles[title] + '\n')
                                    file.write(TITLE_BREAK)
                                else:
                                    # Remove extra newline
                                    buffer.pop()
                                break

                        # Copyright update
                        match = re.search(copyright_p, comment)
                        if match:
                            left_just = 14
                            file.write(TITLE_BREAK + SSN)
                            file.write('// ' + filename + '\n' + SSN )
                            file.write('// @author'.ljust(left_just) + 'Tyler Graveline' + '\n')
                            file.write('// @version'.ljust(left_just) + '2.0' + '\n')
                            file.write('// @date'.ljust(left_just) + match.groupdict()['start'] + '-2020' + '\n')
                            file.write('// @copyright'.ljust(left_just) + 'Copyright Silveryte Studios' + '\n' + SSN)
                            file.write(TITLE_BREAK)

                            if buffer[-1] != '\n':
                                file.write('\n')

                    # Function comment blocks
                    try:
                        match = re.match(function_p, buffer[-1])
                    except:
                        match = None
                        pass
                    if match:
                        constructor = False
                        file.write(FUNCTION_BREAK)

                        # Constructor check
                        if buffer[-1] == '{func}::{func}()\n'.format(func=match.groupdict()['function']):
                            file.write('* @brief Default constructor\n')
                            constructor = True
                        else:
                            # Function specific descriptions
                            brief_description = ''
                            if match.groupdict()['function'] == 'BeginPlay':
                                brief_description = 'Called once actor has been spawned into world'
                            elif 'Tick' in match.groupdict()['function']:
                                brief_description = 'Called every frame'
                            elif match.groupdict()['function'] == 'OnConstruction':
                                brief_description = 'Called after spawning actor but before play'
                            elif match.groupdict()['function'] == 'EndPlay':
                                brief_description = 'Called when actor is being removed from world'
                            else:
                                for comment in comments:
                                    if not re.match(whitespace_p, comment) and \
                                       not comment.strip() == match.groupdict()['function']:
                                        brief_description = comment.strip() + ' '

                            file.write('* @brief {}\n'.format(brief_description))

                        # Param list
                        params = match.groupdict()['params']
                        if len(params):
                            file.write(STAR)
                            param_names = [param for param in params.split() if param != 'const' and param != 'class']
                            for i in range(1, len(param_names), 2):
                                file.write('* @param {} \n'.format(param_names[i].strip(',*&')))

                        print(match.groupdict())
                        if match.groupdict()['return'] != 'void' and not constructor:
                            file.write(STAR)
                            file.write('* @return \n')

                        file.write('*/' + '\n')
                    print(comments)

                # Closing bracket function name
                elif re.match(closing_bracket_p, line):
                    match = re.match(closing_bracket_p, line)
                    file.write('{bracket} // {func}\n'.format(bracket='}', func=match.groupdict()['function']))

                else:
                    file.write(line)

        # Meta pass
        with open(new_filename, 'r') as file:
            buffer = file.readlines()

        with open(new_filename, 'w') as file:
            globals_section = ''
            globals_swapped = False
            method_types = ['Constructors', 'Overrides', 'Accessors', 'Mutators', 'Helpers', 'Mechanics', 'Wrappers']
            method_section = False
            buffer.reverse()
            while buffer:
                line = buffer.pop()

                # Change TODO to @todo
                match = re.search( todo_p, line)
                if match:
                    line = todo_p.sub(r'\g<start>@todo\g<end>', line)
                    file.write(line)

                # Switch order of types and globals
                elif line == TITLE_BREAK and buffer and 'Globals' in buffer[-1]:
                    globals_section = globals_section + TITLE_BREAK + buffer.pop() + buffer.pop()
                    while buffer[-1] != TITLE_BREAK:
                        globals_section = globals_section + buffer.pop()

                # Update Members to new style
                elif line == TITLE_BREAK and buffer and 'Members' in buffer[-1]:
                    file.write(line + buffer.pop() + buffer.pop())

                    while buffer:
                        line = buffer.pop()
                        methods_section = (line == TITLE_BREAK and 'Methods' in buffer[-1])
                        if methods_section:
                            file.write(line)
                            break
                        else:
                            match = re.match(member_p, line)
                            if match:
                                if match.groupdict()['comment'] is not None:
                                    file.write('    ' + '///' + match.groupdict()['comment'].strip('/'))
                                else:
                                    file.write('    ' + '/// \n')

                                file.write('    ')
                                # write uproperty
                                if re.match(ua_p, line):
                                    file.write('UPROPERTY() ')

                                # Write member
                                pieces = match.groupdict()['member'].split()
                                for i in range(0, len(pieces)):
                                    if i == len(pieces) - 1:
                                        file.write(pieces[i])
                                    else:
                                        file.write(pieces[i] + ' ')
                                #[file.write(piece + ' ') for piece in match.groupdict()['member'].split() if True]
                                file.write(';\n')

                                # Make sure newline between each member
                                if buffer[-1] != '\n':
                                    file.write('\n')
                            else:
                                file.write(line)

                # Add method types
                elif not source and 'Methods' in line:
                    file.write(line + buffer.pop())

                    for method_type in method_types:
                        file.write('    // {type}\n\n'.format(type=method_type))
                        method_section = True

                elif method_section and '//' in line:
                    found = False
                    for method_type in method_types:
                        if method_type in line:
                            found = True
                            break
                    if not found:
                        file.write(line)

                # Write the line
                else:
                    if line == TITLE_BREAK and buffer and ('Methods' in buffer[-1] or 'Class Definition' in buffer[-1]) and not globals_swapped:
                        file.write(globals_section)
                        globals_swapped = True

                    file.write(line)
