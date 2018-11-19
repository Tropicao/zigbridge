project('Zigbridge' , 'c')

# Sources
src = ['src/main.c',
        'src/core.c',
        'src/interfaces/http_parser/http_parser.c',
        'src/interfaces/http_server.c',
        'src/interfaces/ipc.c',
        'src/interfaces/stdin.c',
        'src/aps.c',
        'src/conf.c',
        'src/keys.c',
        'src/logs.c',
        'src/rpc/rpc.c',
        'src/mt/mt.c',
        'src/mt/mt_sys.c',
        'src/mt/mt_af.c',
        'src/mt/mt_zdo.c',
        'src/mt/mt_util.c',
        'src/profiles/zdp.c',
        'src/profiles/zha.c',
        'src/profiles/zll.c',
        'src/utils/sm.c',
        'src/utils/action_list.c',
        'src/devices/device.c']

# Includes
incdir = include_directories(  'src',
                                'src/utils',
                                'src/devices',
                                'src/profiles',
                                'src/interfaces',
                                'src/interfaces/http_parser',
                                'src/mt',
                                'src/rpc')

# Libraries
cc = meson.get_compiler('c')
uvdep = dependency('libuv')
iniparserdep = cc.find_library('iniparser')
einadep = dependency('eina')
janssondep = dependency('jansson', version:'>=2.10')
dep = [uvdep, iniparserdep, einadep, janssondep]

# Build options
cflags=['-Wall', '-Wextra', '-Werror']

executable('zigbridge',
    sources: src,
    c_args: cflags,
    include_directories: incdir,
    dependencies: dep,
    install : true)
