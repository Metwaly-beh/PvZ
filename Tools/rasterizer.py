import PIL.Image as Image
import PIL.ImageDraw as ImageDraw
import sys

def render_dump(dump_file, atlas_path, output_path, width=1280, height=720):
    # Load atlas
    atlas = Image.open(atlas_path).convert("RGBA")
    atlas_w, atlas_h = atlas.size

    # Create blank canvas
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))

    with open(dump_file, "r") as f:
        lines = f.readlines()

    # Parse header
    view_height = 0.0
    verts_count = 0
    line_idx = 0
    while line_idx < len(lines):
        line = lines[line_idx].strip()
        if line.startswith("viewHeight"):
            view_height = float(line.split()[1])
        elif line.startswith("vertices"):
            verts_count = int(line.split()[1])
            line_idx += 1
            break
        line_idx += 1

    # The dump contains vertices. A "quad" is 6 vertices (2 triangles).
    # We'll use a very simple software rasterizer:
    # Since we don't have a full GPU, we'll just draw the quads using PIL.
    # Note: The dump is flat vertices. We need to group them.

    all_verts = []
    for i in range(verts_count):
        parts = lines[line_idx + i].split()
        all_verts.append([float(x) for x in parts])

    # Simple approach: the GL pipeline uses triangles.
    # For the purpose of verification, we'll group vertices into 3s.
    # This is naive but works for simple quads if the order is consistent.

    def world_to_pixel(x, y, vh):
        # Projection: ortho(0, 24, 0, vh, -1, 1)
        # x: [0, 24] -> [0, width]
        # y: [0, vh] -> [height, 0] (since pixel (0,0) is top-left)
        px = (x / 24.0) * width
        py = height - (y / vh) * height
        return px, py

    # For verification, we'll just treat every 6 vertices as a quad (2 tris)
    # and draw it. This is a bit hacky but we just need to see the sprites.
    for i in range(0, len(all_verts), 6):
        if i + 5 >= len(all_verts): break

        # Get a bounding box of the 6 vertices
        xs = [v[0] for v in all_verts[i:i+6]]
        ys = [v[1] for v in all_verts[i:i+6]]

        x_min, x_max = min(xs), max(xs)
        y_min, y_max = min(ys), max(ys)

        # Map to pixels
        px_min, py_max = world_to_pixel(x_min, y_max, view_height)
        px_max, py_min = world_to_pixel(x_max, y_min, view_height)

        # Get UVs of the quad (from the first 4 verts usually)
        u0 = all_verts[i][2]
        v0 = all_verts[i][3]
        u1 = all_verts[i+3][2] # Roughly opposite corner
        v1 = all_verts[i+3][3]

        # Atlas coordinates
        ax0 = int(min(u0, u1) * atlas_w)
        ay0 = int(min(v0, v1) * atlas_h)
        ax1 = int(max(u0, u1) * atlas_w)
        ay1 = int(max(v0, v1) * atlas_h)

        # Crop from atlas
        sprite = atlas.crop((ax0, ay0, ax1, ay1))

        # Scale to the calculated pixel size
        w_px = int(px_max - px_min)
        h_px = int(py_max - py_min)
        if w_px <= 0 or h_px <= 0:
            continue
        sprite = sprite.resize((w_px, h_px), Image.NEAREST)

        # Blend with color (tint)
        # Simplified: just paste it.
        img.paste(sprite, (int(px_min), int(py_min)), sprite)

    img.save(output_path)
    print(f"Rendered dump to {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python rasterizer.py <dump.txt> <atlas.png> <output.png>")
    else:
        render_dump(sys.argv[1], sys.argv[2], sys.argv[3])
