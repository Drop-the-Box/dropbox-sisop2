unlet! skip_defaults_vim
source $HOME/.config/nvim/init.vim
let g:ale_c_clang_options="-I src/include"
let g:ale_cpp_clang_options="-I src/include"
let g:ale_cpp_cc_options = '-std=c++11 -Wall -I src/include'
let b:ale_linters = ['gcc']
