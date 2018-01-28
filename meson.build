project('ZLL Gateway' , 'c')

# Sources
src = ['src/main.c',
        #'src/core.c',
        #'src/ipc.c',
        #'src/aps.c',
        'src/conf.c',
        'src/keys.c',
        'src/logs.c',
        'src/rpc/rpc.c',
        'src/mt/mt.c',
        'src/mt/mt_sys.c',
        #'src/mt/mt_af.c',
        #'src/mt/mt_zdo.c',
        'src/mt/mt_util.c',
        #'src/profiles/zdp.c',
        #'src/profiles/zha.c',
        #'src/profiles/zll.c',
        'src/utils/sm.c',
        'src/utils/action_list.c',
        'src/devices/device.c']

# Includes
znpinc = include_directories('/usr/local/include/libznp')
localinc = include_directories( 'src',
                                'src/utils',
                                'src/devices',
                                #'src/profiles',
                                'src/mt',
                                'src/rpc')
incdir = [znpinc, localinc]

# Libraries
cc = meson.get_compiler('c')
uvdep = cc.find_library('uv', dirs: ['/usr/local/lib', '/usr/lib'])
ptdep = dependency('threads')
iniparserdep = cc.find_library('iniparser')
einadep = dependency('eina')
janssondep = dependency('jansson')
dep = [uvdep, ptdep, iniparserdep, einadep, janssondep]

# Build options
cflags=['-Wall', '-Wextra', '-Werror']

executable('zll_gateway',
    sources: src,
    c_args: cflags,
    include_directories: incdir,
    dependencies: dep)
