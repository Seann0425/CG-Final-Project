## List of modified:
```
(1) Complete Load .obj files of furniture
(2) Complete menu and load object
(3) scene 
->  Add a huge flat plane
->  Integrate bottle and vase object becomed buttom type,
    And It will not appear initally
(4) fixed List of Furniture Menu to the window in the upper right corner
計算過程:
```
1. 先算 window 的最右邊位置(x, y) = (window.x[window這個矩形最左上角的x座標]+window.size, window.y)
2. 因為想要 Menu 跟 window的邊邊留點空隙，所以加上一個參數 EdgeSize, Renew (x,y) <= (x-EdgeSize, y+EdgeSize)
3. OpenGL/imGui 是從左上角開始畫矩形。如果直接把(x, y)丟進繪製的pipeline, opengl會把Menu的左上角當成是(x, y)，導致Menu會超出window大小
=> 所以需加入 Pivot.x = 1 這個參數，
=> FinalMeuPosition.x​ = x −(WindowWidth * Pivot.x)=(螢幕右邊)−(WindowWidth×1.0)=螢幕右邊−Padding−WindowWidth
=> ​FinalMeuPosition.y = y (保持原狀即可)
```
* Sofa source : https://sketchfab.com/3d-models/sofa-94938c47cd574eb3a6672c80e109b8e5
* Tv source : https://sketchfab.com/3d-models/flat-screen-tv-c5be303856cb4fcbaabb1d795639f91c
* Table source : https://sketchfab.com/3d-models/kcdf-gyeongsang-04-82314e9072d6416db854ba70c6c617be
* .fbx to .obj : https://products.groupdocs.app/zh-hant/conversion/fbx-to-obj?taskId=c67ce6aa-cafc-4dbb-9240-8ab72a5cb42c