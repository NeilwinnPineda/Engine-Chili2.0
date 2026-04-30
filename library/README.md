# Asset Library

This folder is the working home for renderer-facing assets that we add locally while building out the engine.

## Current layout

```text
library/
  materials/
    stucco/
      albedo.png
      normal.png
      height.png
```

## Naming convention

For now, keep assets grouped by material or set:

- `library/materials/<material-name>/`
- `library/meshes/<asset-name>/`
- `library/textures/<set-name>/`
- `library/shaders/<shader-name>/` if we start storing authored shader assets later

Within a material set, prefer common map names:

- `albedo.*`
- `normal.*`
- `roughness.*`
- `metallic.*`
- `ao.*`
- `height.*`
- `emissive.*`

## Notes

- This is a source asset library, not a generated-output folder.
- Keep filenames simple and lowercase when we normalize them.
- We can add import/build tooling later once the texture and material path is wired into the renderer.
