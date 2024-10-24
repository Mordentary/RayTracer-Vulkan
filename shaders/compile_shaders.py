import os
import sys
import subprocess
import traceback

def main(shader_source_dir, shader_output_dir, glsl_validator):
    try:
        shader_extensions = ['.vert', '.frag', '.comp']

        shader_source_dir = os.path.normpath(shader_source_dir)
        shader_output_dir = os.path.normpath(shader_output_dir)
        print(f"Shader source directory: {shader_source_dir}")
        print(f"Shader output directory: {shader_output_dir}")
        print(f"Using glslangValidator at: {glsl_validator}")
        print("Starting shader compilation...")

        if not os.path.exists(shader_output_dir):
            os.makedirs(shader_output_dir)

        for root, dirs, files in os.walk(shader_source_dir):
            for file in files:
                if any(file.endswith(ext) for ext in shader_extensions):
                    shader_file = os.path.normpath(os.path.join(root, file))
                    rel_path = os.path.relpath(shader_file, shader_source_dir)
                    output_file = os.path.normpath(os.path.join(shader_output_dir, rel_path)) + '.spv'

                    # Create output directory if it doesn't exist
                    output_dir = os.path.dirname(output_file)
                    if not os.path.exists(output_dir):
                        os.makedirs(output_dir)

                    # Check if the shader needs to be compiled
                    if not os.path.exists(output_file) or \
                    os.path.getmtime(shader_file) > os.path.getmtime(output_file):
                        print(f'Compiling shader: {shader_file} -> {output_file}')
                        cmd = [glsl_validator, '-V', shader_file, '-o', output_file]
                        result = subprocess.run(cmd)
                        if result.returncode != 0:
                            print(f'Error compiling shader {shader_file}')
                            sys.exit(1)
                    else:
                        print(f'Shader up to date: {shader_file}')
    except Exception as e:
        print("An error occurred during shader compilation:")
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print('Usage: compile_shaders.py <shader_source_dir> <shader_output_dir> <glsl_validator>')
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])
