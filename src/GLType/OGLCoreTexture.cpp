#include <gli/gli.hpp>
#include <tools/stb_image.h>
#include <tools/string.h>
#include <GLType/OGLTypes.h>
#include <GLType/OGLCoreTexture.h>

__ImplementSubInterface(OGLCoreTexture, GraphicsTexture)

OGLCoreTexturePtr OGLCoreTexture::Create(GLint width, GLint height, GLenum target, GLenum format, GLuint levels)
{
    assert(levels > 0);
    auto tex = std::make_shared<OGLCoreTexture>();
    if (tex->create(width, height, target, format, levels))
        return tex;
    return nullptr;
}

OGLCoreTexturePtr OGLCoreTexture::Create(const std::string& filename)
{
    auto tex = std::make_shared<OGLCoreTexture>();
    if (tex->create(filename))
        return tex;
    // failed to load image
    assert(false);
    return nullptr;
}

OGLCoreTexture::OGLCoreTexture() :
	m_TextureID(0),
	m_Target(GL_INVALID_ENUM),
	m_Format(GL_INVALID_ENUM),
	m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_MipCount(0)
{
}

OGLCoreTexture::~OGLCoreTexture()
{
	destroy();
}

bool OGLCoreTexture::create(GLint width, GLint height, GLenum target, GLenum format, GLuint levels)
{
	GLuint TextureID = 0;
	glCreateTextures(target, 1, &TextureID);
	glTextureStorage2D(TextureID, levels, format, width, height);

	m_Target = target;
	m_TextureID = TextureID;
	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_Depth = 1;
	m_MipCount = levels;

	return true;
}

bool OGLCoreTexture::create(const GraphicsTextureDesc& desc)
{
    auto filename = desc.getFileName();
    if (!filename.empty())
        return create(filename);

    auto width = desc.getWidth();
    auto height = desc.getHeight();
    auto levels = desc.getLevels();
    auto target = OGLTypes::translate(desc.getTarget());
    auto format = OGLTypes::translate(desc.getFormat());
    return create(width, height, target, format, levels);
}

bool OGLCoreTexture::create(const std::string& filename)
{
    if (filename.empty()) return false;
    std::string ext = util::getFileExtension(filename);
    if (util::stricmp(ext, "DDX") || util::stricmp(ext, "DDS"))
        return createFromFileGLI(filename);
    return createFromFileSTB(filename);
} 

// filename can be KTX or DDS files
bool OGLCoreTexture::createFromFileGLI(const std::string& filename)
{
	gli::texture Texture = gli::load(filename);
	if(Texture.empty())
		return false;

	gli::gl GL(gli::gl::PROFILE_GL33);
	gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
	GLenum Target = GL.translate(Texture.target());

	GLuint TextureID = 0;
	glCreateTextures(Target, 1, &TextureID);
	glTextureParameteri(TextureID, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(TextureID, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
	glTextureParameteri(TextureID, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
	glTextureParameteri(TextureID, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
	glTextureParameteri(TextureID, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
	glTextureParameteri(TextureID, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

	glm::tvec3<GLsizei> const Extent(Texture.extent());
	GLsizei const FaceTotal = static_cast<GLsizei>(Texture.layers() * Texture.faces());

	switch(Texture.target())
	{
	case gli::TARGET_1D:
		glTextureStorage1D(
			TextureID, static_cast<GLint>(Texture.levels()), Format.Internal, Extent.x);
		break;
	case gli::TARGET_1D_ARRAY:
	case gli::TARGET_2D:
	case gli::TARGET_CUBE:
		glTextureStorage2D(
			TextureID, static_cast<GLint>(Texture.levels()), Format.Internal,
			Extent.x, Extent.y);
		break;
	case gli::TARGET_2D_ARRAY:
	case gli::TARGET_3D:
	case gli::TARGET_CUBE_ARRAY:
		glTextureStorage3D(
			TextureID, static_cast<GLint>(Texture.levels()), Format.Internal,
			Extent.x, Extent.y,
			Texture.target() == gli::TARGET_3D ? Extent.z : FaceTotal);
		break;
	default:
		assert(0);
		break;
	}

	for(std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
	for(std::size_t Face = 0; Face < Texture.faces(); ++Face)
	for(std::size_t Level = 0; Level < Texture.levels(); ++Level)
	{
		GLsizei const LayerGL = static_cast<GLsizei>(Layer);
		glm::tvec3<GLsizei> Extent(Texture.extent(Level));
		GLenum Target = gli::is_target_cube(Texture.target())
			? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face)
			: Target;

		switch(Texture.target())
		{
		case gli::TARGET_1D:
			if(gli::is_compressed(Texture.format()))
				glCompressedTextureSubImage1D(
					TextureID, static_cast<GLint>(Level), 0, Extent.x,
					Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
					Texture.data(Layer, Face, Level));
			else
				glTextureSubImage1D(
					TextureID, static_cast<GLint>(Level), 0, Extent.x,
					Format.External, Format.Type,
					Texture.data(Layer, Face, Level));
			break;
		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE:
			if(gli::is_compressed(Texture.format()))
				glCompressedTextureSubImage2D(
					TextureID, static_cast<GLint>(Level),
					0, 0,
					Extent.x,
					Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
					Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
					Texture.data(Layer, Face, Level));
			else
				glTextureSubImage2D(
					TextureID, static_cast<GLint>(Level),
					0, 0,
					Extent.x,
					Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
					Format.External, Format.Type,
					Texture.data(Layer, Face, Level));
			break;
		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY:
			if(gli::is_compressed(Texture.format()))
				glCompressedTextureSubImage3D(
					TextureID, static_cast<GLint>(Level),
					0, 0, 0,
					Extent.x, Extent.y,
					Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
					Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
					Texture.data(Layer, Face, Level));
			else
				glTextureSubImage3D(
					TextureID, static_cast<GLint>(Level),
					0, 0, 0,
					Extent.x, Extent.y,
					Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
					Format.External, Format.Type,
					Texture.data(Layer, Face, Level));
			break;
		default: 
			assert(0); 
			break;
		}
	}
	m_Target = Target;
	m_TextureID = TextureID;
	m_MipCount = static_cast<GLint>(Texture.levels());
	m_Format = Format.Type;
	m_Width = Extent.x;
	m_Height = Extent.y;
	m_Depth = Texture.target() == gli::TARGET_3D ? Extent.z : FaceTotal;

	return true;
}

// filename can be JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC files
bool OGLCoreTexture::createFromFileSTB(const std::string& filename)
{
    stbi_set_flip_vertically_on_load(true);

	GLenum Target = GL_TEXTURE_2D;
    GLenum Type = GL_UNSIGNED_BYTE;
    int Width = 0, Height = 0, nrComponents = 0;
    void* Data = nullptr;
    std::string Ext = util::getFileExtension(filename);
    if (util::stricmp(Ext, "HDR"))
    {
        Type = GL_FLOAT;
        Data = stbi_loadf(filename.c_str(), &Width, &Height, &nrComponents, 0);
    }
    else
    {
        Data = stbi_load(filename.c_str(), &Width, &Height, &nrComponents, 0);
    }
    if (!Data) return false;

    GLenum Format = OGLTypes::getComponent(nrComponents);
    GLenum InternalFormat = OGLTypes::getInternalComponent(nrComponents, Type == GL_FLOAT);

	GLuint TextureID = 0;
	glCreateTextures(Target, 1, &TextureID);

	// Use fixed storage
    glTextureStorage2D(TextureID, 1, InternalFormat, Width, Height);
    glTextureSubImage2D(TextureID, 0, 0, 0, Width, Height, Format, Type, Data);

    stbi_image_free(Data);

	m_Target = Target;
	m_TextureID = TextureID;
	m_Format = Type;
	m_Width = Width;
	m_Height = Height;
	m_Depth = 1;
	m_MipCount = 1;

	return true;
}

GLuint OGLCoreTexture::getTextureID() const noexcept
{
    return m_TextureID;
}

const GraphicsTextureDesc& OGLCoreTexture::getGraphicsTextureDesc() const noexcept
{
    return m_TextureDesc;
}

void OGLCoreTexture::setDevice(const GraphicsDevicePtr& device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLCoreTexture::getDevice() noexcept
{
    return m_Device.lock();
}

void OGLCoreTexture::destroy()
{
	if (!m_TextureID)
	{
		glDeleteTextures(1, &m_TextureID);
		m_TextureID = 0;

        m_Format = GL_INVALID_ENUM;
        m_Width = 0;
        m_Height = 0;
        m_Depth = 0;
        m_MipCount = 0;
	}
	m_Target = GL_INVALID_ENUM;
}

void OGLCoreTexture::bind(GLuint unit) const
{
	assert( 0u != m_TextureID );  
    glBindTextureUnit(unit, m_TextureID);
}

void OGLCoreTexture::unbind(GLuint unit) const
{
    glBindTextureUnit(unit, 0);
}

void OGLCoreTexture::generateMipmap()
{
	assert(m_Target != GL_INVALID_ENUM);
	assert(m_TextureID != 0);

	glGenerateTextureMipmap(m_TextureID);
}

void OGLCoreTexture::parameter(GLenum pname, GLint param)
{
	assert(m_Target != GL_INVALID_ENUM);
	assert(m_TextureID != 0);

    glTextureParameteri(m_TextureID, pname, param);
}
