## List of Modified Features

1. Complete Load .obj files of furniture
2. Complete menu and load object
3. Scene improvements:
   -Add a huge flat plane
   -Integrate bottle and vase objects as button type
   -Make These objects will not appear initially
4. Fixed List of Furniture Menu to the window in the upper right corner

### Menu Position Calculation

1. Calculate window's rightmost position: `(x, y) = (window.x + window.size, window.y)`
   - `window.x` is the x-coordinate of the window's top-left corner

2. Add padding between Menu and window edge:
   - Add parameter `EdgeSize`
   - Renew position: `(x, y) <= (x - EdgeSize, y + EdgeSize)`

3. OpenGL/ImGui draws rectangles from top-left corner:
   - If we directly use `(x, y)` in the rendering pipeline, OpenGL will treat Menu's top-left as `(x, y)`, causeing Menu to exceed window size
   - Solution: Add `Pivot.x = 1` parameter
   - `FinalMenuPosition.x = x - (WindowWidth × Pivot.x) = (Screen Right) - (WindowWidth × 1.0) = Screen Right - Padding - WindowWidth`
   - `FinalMenuPosition.y = y` (keep original)

## Resources

* Sofa source: https://sketchfab.com/3d-models/sofa-94938c47cd574eb3a6672c80e109b8e5
* TV source: https://sketchfab.com/3d-models/flat-screen-tv-c5be303856cb4fcbaabb1d795639f91c
* Table source: https://sketchfab.com/3d-models/kcdf-gyeongsang-04-82314e9072d6416db854ba70c6c617be
* .fbx to .obj converter: https://products.groupdocs.app/zh-hant/conversion/fbx-to-obj