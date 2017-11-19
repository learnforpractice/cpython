
def find_redefines():
    redefined = {}
    with open('/Users/newworld/dev/pyeos/libraries/tinypy/redefine.txt','r') as f:
        lines = f.readlines()
        for line in lines:
            if line.find('macro redefined [-Wmacro-redefined]') < 0:
                continue
            start_str = "warning: '"
            start = line.find(start_str)
            if start < 0:
                continue
            start += len(start_str)
            end = line.find("'",start)
            if end < 0:
                continue
            line = line[start:end]
            if not line in redefined:
                redefined[line] = line
                print(line)
#            print(line)
    return redefined

def redefine_symbols():
#    redefines = find_redefines()
    redefines = {}
    with open('/Users/newworld/dev/pyeos/libraries/tinypy/nm.txt','r') as f:
        lines = f.readlines()
        for line in lines:
            if line[0] != '0':
                continue
#            print(line)
#            print(line[:18][-2:])
            data_type = line[:18][-2:]
            if data_type in [' C', ' T', ' D', ' S']:
                pass
            else:
                continue

            start = line.rfind(' ')
            line = line[start+2:-1]
            if line in redefines:
                continue
            print('#define %-30s tiny_%s %20s'%(line, line, "//"+data_type))

redefine_symbols()


                
                
                