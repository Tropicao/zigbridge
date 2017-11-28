project('ZLL Gateway' , 'c')

src = ['src/main.c']

cc = meson.get_compiler('c')
add_global_arguments('-DCFLAGS=-Wall -Wextra -Werror', language: 'c')
dep = cc.find_library('znp', dirs: ['/usr/local/lib', '/usr/lib'])

executable('zll_gateway',
    sources: src,
    dependencies: dep)
