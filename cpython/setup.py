from distutils.core import setup, Extension #这里要用到distutils库
module1 = Extension('Test', sources = ['test.c']) #打包文件为test.c，这里没有设置路径，所有.c文件和setup.py放在同一目录下
setup (name = 'Test', #打包名
       version = '1.0', #版本
       description = 'This is a demo package', #说明文字
       ext_modules = [module1])
