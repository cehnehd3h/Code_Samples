/* ---------------------------------------------------------------------------
** Team Bear King
** © 2015 DigiPen Institute of Technology, All Rights Reserved.
**
** DXBufferManager.cpp
**
** Author:
** - Matt Yan - m.yan@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** -------------------------------------------------------------------------*/

#include "UrsinePrecompiled.h"
#include "ShaderBufferManager.h"
#include <d3d11.h>
#include "DXErrorHandling.h"


namespace ursine
{
    namespace graphics
    {
        namespace DXCore
        {
            /////////////////////////////////////////////////////////////////////////////
            // Initializes all of the buffers that will be used for rendering
            void ShaderBufferManager::Initialize(
                ID3D11Device *device,
                ID3D11DeviceContext *context
                )
            {
                //save the pointers since we'll be using them a lot
                m_device = device;
                m_deviceContext = context;

                //init
                m_bufferArray.resize( BUFFER_COUNT );


                // make all the buffers. The template parameter is the buffer struct,
                    // with the function parameter is the enum which will represent 
                    // the buffer
                MakeBuffer<CameraBuffer>( BUFFER_CAMERA );
                MakeBuffer<TransformBuffer>( BUFFER_TRANSFORM );
                MakeBuffer<DirectionalLightBuffer>( BUFFER_DIRECTIONAL_LIGHT );
                MakeBuffer<PointLightBuffer>( BUFFER_POINT_LIGHT );
                MakeBuffer<InvProjBuffer>( BUFFER_INV_PROJ );
                MakeBuffer<PrimitiveColorBuffer>( BUFFER_PRIM_COLOR );
                MakeBuffer<PointGeometryBuffer>( BUFFER_POINT_GEOM );
                MakeBuffer<BillboardSpriteBuffer>( BUFFER_BILLBOARDSPRITE );
                MakeBuffer<GBufferUnpackBuffer>( BUFFER_GBUFFER_UNPACK );
                MakeBuffer<TransformBuffer>( BUFFER_LIGHT_PROJ );
                MakeBuffer<MaterialDataBuffer>( BUFFER_MATERIAL_DATA );
                MakeBuffer<SpotlightBuffer>( BUFFER_SPOTLIGHT );
                MakeBuffer<MouseBuffer>( BUFFER_MOUSEPOS );
            }

            /////////////////////////////////////////////////////////////////////////////
            // Releases all resources and buffers, as well as setting them to nullptrs to 
            // avoid direct x memory leaks
            void ShaderBufferManager::Uninitialize( void )
            {
                //release the buffer. This checks to make sure the resource isn't null,  
                    // releases it if available, and sets to null
                for ( unsigned x = 0; x < BUFFER_COUNT; ++x )
                {
                    RELEASE_RESOURCE( m_bufferArray[ x ] );
                }

                //if we don't set these to null, we will get warnings from directx
                m_device = nullptr;
                m_deviceContext = nullptr;
            }

            /////////////////////////////////////////////////////////////////////////////
            // Instead of having different setters for each type of buffer, this 
            // templated function uses memcopy to copy any type of data, allowing for 
            // extreme flexibility
            template<BUFFER_LIST buffer, typename T>
            void MapBuffer(
                const T *data,
                const SHADERTYPE shader,
                const unsigned int bufferIndex = buffer
                )
            {
                // ensure that we don't attempt to set a buffer index that doesn't exist
                UAssert( bufferIndex < MAX_CONST_BUFF, "ResourceManager attempted to map 
                    buffer to invalid index( index #%i )", bufferIndex);

                    HRESULT result;
                D3D11_MAPPED_SUBRESOURCE mappedResource;

                T *dataPtr;

                // check to see if the buffer was actually created
                    // make sure buffer exists
                UAssert( m_bufferArray[ buffer ] != nullptr,
                    "A buffer was never initialized!" );

                // lock the buffer for writing. We will use WRITE_DISCARD, as we don't 
                    // care about previous information
                result = m_deviceContext->Map(
                    m_bufferArray[ buffer ],
                    0,
                    D3D11_MAP_WRITE_DISCARD,
                    0,
                    &mappedResource
                    );

                // grab a pointer to the data
                dataPtr = reinterpret_cast<T*>(mappedResource.pData);

                // copy data from the input buffer into the GPU buffer
                memcpy(
                    dataPtr,
                    data,
                    sizeof( T )
                    );

                // unlock buffer, sending data back to GPU
                m_deviceContext->Unmap(
                    m_bufferArray[ buffer ],
                    0
                    );

                // map buffer to a given index and shader
                SetBuffer(
                    shader,
                    bufferIndex,
                    m_bufferArray[ buffer ]
                    );
            }

            /////////////////////////////////////////////////////////////////////////////
            // PRIVATE METHODS //////////////////////////////////////////////////////////

            /////////////////////////////////////////////////////////////////////////////
            // this function takes the buffer, a shader type (pixel, vertex, etc), and an 
            // index, then sets it on the GPU.
            void ShaderBufferManager::SetBuffer(
                const SHADERTYPE shader,
                const unsigned bufferIndex,
                ID3D11Buffer *buffer
                )
            {
                // what shader are we using? vertex? geometry?
                switch ( shader ) {
                case SHADERTYPE_VERTEX:
                    m_deviceContext->VSSetConstantBuffers(
                        bufferIndex,
                        1,
                        &buffer
                        );
                    break;
                case SHADERTYPE_PIXEL:
                    m_deviceContext->PSSetConstantBuffers(
                        bufferIndex,
                        1,
                        &buffer
                        );
                    break;
                case SHADERTYPE_HULL:
                    m_deviceContext->HSSetConstantBuffers(
                        bufferIndex,
                        1,
                        &buffer
                        );
                    break;
                case SHADERTYPE_DOMAIN:
                    m_deviceContext->DSSetConstantBuffers(
                        bufferIndex,
                        1,
                        &buffer
                        );
                    break;
                case SHADERTYPE_GEOMETRY:
                    m_deviceContext->GSSetConstantBuffers(
                        bufferIndex,
                        1,
                        &buffer
                        );
                    break;
                case SHADERTYPE_COMPUTE:
                    m_deviceContext->CSSetConstantBuffers(
                        bufferIndex,
                        1,
                        &buffer
                        );
                    break;
                default:
                    // we shouldn't ever hit here, but it's better to catch this
                    UAssert( false, "Failed to set buffer!" );
                    break;
                }
            }


            template<typename T>
            void ShaderBufferManager::MakeBuffer(
                const BUFFER_LIST type,
                unsigned gpuUsage,
                unsigned cpuAccess
                )
            {
                // all buffers *must* have a size that is a multiple of 16
                UAssert( sizeof( T ) % 16 == 0, "Invalid constant buffer! Constant buffer 
                    must have a multiple of 16 as its bit width!");

                    HRESULT result;
                D3D11_BUFFER_DESC bufferDesc;

                // how to use this? is it for shaders?
                bufferDesc.Usage = static_cast<D3D11_USAGE>(gpuUsage);

                // how big is this buffer
                bufferDesc.ByteWidth = sizeof( T );

                // how will it be used?
                bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

                // how does the CPU access this?
                bufferDesc.CPUAccessFlags = cpuAccess;

                // we aren't using any structured buffers and this isn't a UAV, 
                    // so we won't worry about these flags
                bufferDesc.MiscFlags = 0;
                bufferDesc.StructureByteStride = 0;

                //Create the constant buffer pointer so we can access the shader constant
                    // buffer from within this class.
                result = m_device->CreateBuffer(
                    &bufferDesc,
                    nullptr,
                    &m_bufferArray[ type ]
                    );

                UAssert(
                    result == S_OK,
                    "Failed to make buffer! (type: %i)  (Error '%s')",
                    type,
                    GetDXErrorMessage( result )
                    );
            }
        }
    }
}