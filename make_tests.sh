#mv ./packaging/tests.nspec ./packaging/tests.spec
#mv ./packaging/xorg-x11-drv-exynos.spec ./packaging/xorg-x11-drv-exynos.nspec
#gbs build -A i586 --include-all --noinit

gbs build -A i586 --include-all --noinit --spec tests.spec
cp ../GBS-ROOT-TIZEN/local/BUILD-ROOTS/scratch.i586.0/home/abuild/rpmbuild/BUILD/test_gtest-1.1/test . 
