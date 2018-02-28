#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLUtil.h>
#include <cassert>

__ImplementSubInterface(OGLGraphicsData, GraphicsData, "OGLGraphicsData")

OGLGraphicsData::OGLGraphicsData() noexcept
    : m_BufferID(GL_NONE)
    , m_Target(GL_NONE)
{
}

OGLGraphicsData::~OGLGraphicsData() noexcept
{
    destroy();
}

bool OGLGraphicsData::create(const GraphicsDataDesc& desc) noexcept
{
	assert(m_BufferID == GL_NONE);
    assert(desc.getStreamSize() > 0);
    if (m_BufferID != GL_NONE)
        return false;

    // TODO: Add other type
    auto type = desc.getType();
    if (type == GraphicsDataType::UniformBuffer)
        m_Target =  GL_UNIFORM_BUFFER;
    else
    {
        // TODO: add handler for false case
        assert(false);
		printf("Not supporting graphics data types.");
        return false;
    }

    // TODO: Need to check flag options
	glGenBuffers(1, &m_BufferID);
	glBindBuffer(m_Target, m_BufferID);
	glBufferData(m_Target, desc.getStreamSize(), desc.getStream(), GL_DYNAMIC_DRAW);

	if (m_BufferID == GL_NONE)
	{
		printf("glGenBuffers() fail.");
		return false;
	}

	m_Desc = desc;
    return true;
}

void OGLGraphicsData::destroy() noexcept
{
    m_Target = GL_NONE;
    if (m_BufferID)
    {
        glDeleteBuffers(1, &m_BufferID);
        m_BufferID = GL_NONE;
    }
}

bool OGLGraphicsData::map(std::ptrdiff_t offset, std::ptrdiff_t count, void** data, GraphicsUsageFlags flags) noexcept
{
	assert(data);
    // Check if flags match with creation usages
    assert(false);
	glBindBuffer(m_Target, m_BufferID);
    // TODO: Check flag options, just copied from OGLCore
	*data = glMapBufferRange(m_Target, offset, count, GetOGLUsageFlag(flags));
	return *data ? true : false;
}

void OGLGraphicsData::unmap() noexcept
{
    glBindBuffer(m_Target, m_BufferID);
    glUnmapBuffer(m_Target);
}

void OGLGraphicsData::update(std::ptrdiff_t offset, std::ptrdiff_t count, void* data)
{
    glBindBuffer(m_Target, m_BufferID);
    glBufferSubData(m_Target, offset, count, data);
}

GLuint OGLGraphicsData::getInstanceID() const noexcept
{
    assert(m_BufferID != GL_NONE);
    return m_BufferID;
}

void OGLGraphicsData::setDevice(GraphicsDevicePtr device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLGraphicsData::getDevice() noexcept
{
    return m_Device.lock();
}
