建立一个易于管理的Windows VC++工程

1 优化临时文件目录路径

```
$(SolutionDir)../temp/$(Platform)/$(Configuration)/$(ProjectName)
```

2 优化输出文件目录路径

```
$(SolutionDir)../bin/$(Platform)/$(Configuration)
```





