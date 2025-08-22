//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <SDL3_shadercross/SDL_shadercross.h>


void shader_importer_import(const path_t* source_path, const path_t* output_path)
{
}

bool shader_importer_can_import(const path_t* path)
{
    if (path_has_extension(path, ".hlsl"))
        return true;

    return false;
}

bool shader_importer_does_depend_on(const path_t* source_path, const path_t* dependency_path)
{
    // For now, shader files don't have dependencies
    // This could be extended to check for #include files
    return false;
}

static asset_importer_traits_t shader_importer_traits = {
    .can_import = shader_importer_can_import,
    .import_func = shader_importer_import,
    .does_depend_on = shader_importer_does_depend_on
};

asset_importer_traits_t* shader_importer_create()
{
    return &shader_importer_traits;
}


#if 0
    bool ShaderImporter::can_import(const string& filePath) const
    {
        filesystem::path path(filePath);
        string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return tolower(c); });
        return extension == ".hlsl";
    }

    void ShaderImporter::import(const string& source_filename, const string& output_dir)
    {
        filesystem::path source_path(source_filename);
        auto output_name = source_path.stem().string() + ".shader";
        auto output_path = filesystem::path(output_dir) / output_name;
        auto meta = meta_file::parse(source_filename + ".meta");

        // Read source shader file
        ifstream source_file(source_path);
        if (!source_file.is_open())
            throw runtime_error("failed to open source file");

        stringstream source_stream;
        source_stream << source_file.rdbuf();
        auto source = source_stream.str();
        source_file.close();

        // Parse vertex shader
        auto vertex_shader = parse_shader(source, source_path, "VERTEX_SHADER");
        validate_vertex_shader(vertex_shader);

        // Parse fragment shader
        auto fragment_shader = parse_shader(source, source_path, "FRAGMENT_SHADER");
        validate_fragment_shader(fragment_shader);

        // Write the shader
        auto includeDir = filesystem::path(source_path).parent_path().string();
        binary_stream stream;
        write_shader(source_path.string(), vertex_shader, fragment_shader, meta, includeDir, stream);
        stream.save(output_path.string());
    }

    ShaderImporter::shader_info ShaderImporter::parse_shader(const string& source, const filesystem::path& source_path, const string& stage)
    {
        shader_info info = {};
        info.source = preprocess_includes(preprocess_stage_directives(source, stage), source_path);

        // Convert to lowercase for case-insensitive matching
        string code = info.source;
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return tolower(c); });

        // Regex patterns for different resource types
        static regex cbufferPattern(R"(cbuffer.*?register\s*\(\s*b(\d+))");
        static regex samplerPattern(R"(sampler.*?register\s*\(\s*s(\d+))");

        // Count cbuffers and categorize by binding type
        sregex_iterator cbufferIter(code.begin(), code.end(), cbufferPattern);
        sregex_iterator cbufferEnd;
        for (; cbufferIter != cbufferEnd; ++cbufferIter)
        {
            auto index = stoi((*cbufferIter)[1].str());
            if (index > 15)
                throw runtime_error("Invalid vertex uniform buffer index: " + to_string(index));

            info.uniforms[index] = true;
        }

        // Count samplers
        sregex_iterator samplerIter(code.begin(), code.end(), samplerPattern);
        sregex_iterator samplerEnd;
        for (; samplerIter != samplerEnd; ++samplerIter)
        {
            auto index = stoi((*samplerIter)[1].str());
            if (index > 15)
                throw runtime_error("Invalid sampler buffer index: " + to_string(index));

            info.samplers[index] = true;
        }

        return info;
    }

    string ShaderImporter::preprocess_includes(const string& source, const filesystem::path& source_path)
    {
        string result = source;
        regex include_pattern("#include\\s*[\"<]([^\">]+)[\">]");

        sregex_iterator iter(result.begin(), result.end(), include_pattern);
        sregex_iterator end;

        // Process includes in reverse order to maintain line numbers
        vector<pair<string, string>> includes;
        for (; iter != end; ++iter)
        {
            auto include_path = (*iter)[1].str();
            auto full_include_path = resolve_include_path(include_path, source_path);
            includes.push_back({ (*iter)[0].str(), full_include_path });
        }

        // Replace includes with their content
        for (const auto& include : includes)
        {
            string includeContent = load_include_file(include.second);
            if (!includeContent.empty())
            {
                // Replace the include directive with the content
                size_t pos = result.find(include.first);
                if (pos != string::npos)
                {
                    result.replace(pos, include.first.length(), includeContent);
                }
            }
        }

        return result;
    }

    string ShaderImporter::preprocess_stage_directives(const string& source, const string& stage)
    {
        string result = source;

        // Convert custom stage directives to #ifdef blocks based on current stage
        // Format: //@ VERTEX ... //@ END and //@ FRAGMENT ... //@ END

        // Use [\s\S] instead of . to match newlines (equivalent to dotall)
        regex vertexPattern(R"(//@ VERTEX\s*\n([\s\S]*?)//@ END)");
        regex fragmentPattern(R"(//@ FRAGMENT\s*\n([\s\S]*?)//@ END)");

        if (stage == "VERTEX_SHADER")
        {
            // Keep vertex shader blocks, remove fragment shader blocks
            result = regex_replace(result, vertexPattern, "$1");
            result = regex_replace(result, fragmentPattern, "");
        }
        else if (stage == "FRAGMENT_SHADER")
        {
            // Keep fragment shader blocks, remove vertex shader blocks
            result = regex_replace(result, fragmentPattern, "$1");
            result = regex_replace(result, vertexPattern, "");
        }

        return result;
    }

    string ShaderImporter::resolve_include_path(const string& include_path, const filesystem::path& source_path)
    {
        auto source_dir = filesystem::path(source_path).parent_path();
        auto relative_path = source_dir / include_path;

        if (filesystem::exists(relative_path))
            return relative_path.string();

        for (const auto& includeDir : _config.includePaths)
        {
            filesystem::path configPath = filesystem::path(includeDir) / include_path;
            if (filesystem::exists(configPath))
            {
                return configPath.string();
            }
        }

        return include_path;
    }

    string ShaderImporter::load_include_file(const filesystem::path& include_path)
    {
        ifstream include_file(include_path);
        if (!include_file.is_open())
            throw std::runtime_error("Failed to open include file: " + include_path.string());

        stringstream content;
        content << include_file.rdbuf();
        include_file.close();
        return content.str();
    }

    void ShaderImporter::validate_vertex_shader(const shader_info& shader) const
    {
        auto code = shader.source;
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return tolower(c); });

        if (code.find("vs(") == string::npos)
            throw runtime_error("Vertex shader must contain a 'vs' entry point");
    }

    void ShaderImporter::validate_fragment_shader(const shader_info& shader) const
    {
        auto code = shader.source;
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return tolower(c); });

        if (code.find("ps(") == string::npos)
            throw runtime_error("Fragment shader must contain a 'ps' entry point");
    }

    void ShaderImporter::write_shader(
        const string& source_path,
        const shader_info& vs,
        const shader_info& fs,
        const meta_file& meta,
        const string& includeDir,
        binary_stream& stream)
    {
        auto flags = shader_flags::none;

        if (meta.get_bool("shader", "depth_test", true))
            flags = static_cast<shader_flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(shader_flags::depth_test));

        if (meta.get_bool("shader", "depth_write", true))
            flags = static_cast<shader_flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(shader_flags::depth_write));

        if (meta.get_bool("shader", "blend_enabled", true))
            flags = static_cast<shader_flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(shader_flags::blend));

        // src_blend_factor
        auto srcBlend = meta.get_string("shader", "src_blend_factor", "one");
        SDL_GPUBlendFactor srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
        if (srcBlend == "one") srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
        else if (srcBlend == "zero") srcBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
        else if (srcBlend == "src_alpha") srcBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        else if (srcBlend == "one_minus_src_alpha") srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

        // dst_blend_factor
        auto dstBlend = meta.get_string("shader", "dst_blend_factor", "zero");
        SDL_GPUBlendFactor dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
        if (dstBlend == "one") dstBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
        else if (dstBlend == "zero") dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
        else if (dstBlend == "src_alpha") dstBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        else if (dstBlend == "one_minus_src_alpha") dstBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

        // cull_mode
        auto cullMode = meta.get_string("shader", "cull", "none");
        SDL_GPUCullMode cullModeValue = SDL_GPU_CULLMODE_NONE;
        if (cullMode == "none") cullModeValue = SDL_GPU_CULLMODE_NONE;
        else if (cullMode == "front") cullModeValue = SDL_GPU_CULLMODE_FRONT;
        else if (cullMode == "back") cullModeValue = SDL_GPU_CULLMODE_BACK;

        // Compile shaders to SPIR-V
        void* vertexBytecode = nullptr;
        size_t vertexBytecodeSize = 0;
        void* fragmentBytecode = nullptr;
        size_t fragmentBytecodeSize = 0;

        // Compile vertex shader
        SDL_ShaderCross_HLSL_Info vertex_info = {};
        vertex_info.source = vs.source.c_str();
        vertex_info.name = source_path.c_str();
        vertex_info.entrypoint = "vs";
        vertex_info.include_dir = includeDir.c_str();
        vertex_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;

        vertexBytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&vertex_info, &vertexBytecodeSize);
        if (!vertexBytecode)
            throw runtime_error(SDL_GetError());

        // Compile fragment shader
        SDL_ShaderCross_HLSL_Info fragment_info = {};
        fragment_info.source = fs.source.c_str();
        fragment_info.name = source_path.c_str();
        fragment_info.entrypoint = "ps";
        fragment_info.include_dir = includeDir.c_str();
        fragment_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;

        fragmentBytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&fragment_info, &fragmentBytecodeSize);
        if (!fragmentBytecode)
            throw runtime_error(SDL_GetError());

        stream.write_signature("SHDR");
        stream.write_uint32(1);
        stream.write_bytes((uint8_t*)vertexBytecode, (uint16_t)vertexBytecodeSize);
        stream.write_bytes((uint8_t*)fragmentBytecode, (uint16_t)fragmentBytecodeSize);

        // Write resource counts
        stream.write_int32(
            vs.uniforms[static_cast<int>(vertex_register::user0)] +
            vs.uniforms[static_cast<int>(vertex_register::user1)] +
            vs.uniforms[static_cast<int>(vertex_register::user2)]);
        stream.write_int32(
            fs.uniforms[static_cast<int>(fragment_register::user0)] +
            fs.uniforms[static_cast<int>(fragment_register::user1)] +
            fs.uniforms[static_cast<int>(fragment_register::user2)]);
        stream.write_int32(
            fs.samplers[static_cast<int>(sampler_register::user0)] +
            fs.samplers[static_cast<int>(sampler_register::user1)] +
            fs.samplers[static_cast<int>(sampler_register::user2)]);

        // Write pipeline properties
        stream.write_uint8(static_cast<uint8_t>(flags));
        stream.write_uint32(srcBlendFactor);
        stream.write_uint32(dstBlendFactor);
        stream.write_uint32(cullModeValue);

        SDL_free(vertexBytecode);
        SDL_free(fragmentBytecode);
    }

    vector<string> ShaderImporter::get_supported_extensions() const
    {
        return { ".hlsl" };
    }

    string ShaderImporter::get_name() const
    {
        return "ShaderImporter";
    }
#endif
