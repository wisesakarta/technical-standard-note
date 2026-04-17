import os
from PIL import Image

def convert_png_to_ico(png_path, ico_base_name):
    if not os.path.exists(png_path):
        print(f"Error: {png_path} not found.")
        return False
    
    output_path = os.path.join("src", f"{ico_base_name}.ico")
    print(f"Converting {png_path} to {output_path}...")
    img = Image.open(png_path).convert('RGBA')
    
    # Standard Windows icon sizes (16px to 256px)
    sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    
    img_w, img_h = img.size
    valid_sizes = [s for s in sizes if s[0] <= img_w]
    if not valid_sizes:
        valid_sizes = [sizes[0]]
        
    img.save(output_path, format='ICO', sizes=valid_sizes)
    print(f"Successfully created {output_path}")
    return True

if __name__ == "__main__":
    icon_dir = os.path.join("assets", "icons")
    
    # 1. Main App Icons
    app_png = os.path.join(icon_dir, "otso_icon.png")
    if os.path.exists(app_png):
        convert_png_to_ico(app_png, "app_icon")
        convert_png_to_ico(app_png, "icon")
    
    # 2. In-App Icon Fallback
    in_app_png = os.path.join(icon_dir, "otso_in_app_icon.png")
    if os.path.exists(in_app_png):
        convert_png_to_ico(in_app_png, "in_app_icon")
        
        # 3. Light Theme
        convert_png_to_ico(in_app_png, "in_app_icon_light")
        
        # 4. Dark Theme (Dedicated Asset)
        dark_png = os.path.join(icon_dir, "otso_in_app_icon_dark.png")
        if os.path.exists(dark_png):
            convert_png_to_ico(dark_png, "in_app_icon_dark")
        else:
            print("Notice: otso_in_app_icon_dark.png not found, using original for dark theme.")
            convert_png_to_ico(in_app_png, "in_app_icon_dark")
