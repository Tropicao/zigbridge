project('ZLL Gateway' , 'c')

src = ['src/main.c']
incdir = include_directories('/usr/local/include/libznp')

cc = meson.get_compiler('c')
cflags=['-Wall', '-Wextra', '-Werror']
znpdep = cc.find_library('znp', dirs: ['/usr/local/lib', '/usr/lib'])
uvdep = cc.find_library('uv', dirs: ['/usr/local/lib', '/usr/lib'])
dep = [znpdep, uvdep]

executable('zll_gateway',
    sources: src,
    c_args: cflags,
    include_directories: incdir,
    dependencies: dep)
