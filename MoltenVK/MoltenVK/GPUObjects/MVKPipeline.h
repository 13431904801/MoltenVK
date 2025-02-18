/*
 * MVKPipeline.h
 *
 * Copyright (c) 2014-2019 The Brenwill Workshop Ltd. (http://www.brenwill.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "MVKDevice.h"
#include "MVKDescriptorSet.h"
#include "MVKShaderModule.h"
#include "MVKSync.h"
#include "MVKVector.h"
#include <MoltenVKSPIRVToMSLConverter/SPIRVReflection.h>
#include <MoltenVKSPIRVToMSLConverter/SPIRVToMSLConverter.h>
#include <unordered_set>
#include <ostream>

#import <Metal/Metal.h>

class MVKCommandEncoder;
class MVKPipelineCache;


#pragma mark -
#pragma mark MVKPipelineLayout

struct MVKShaderImplicitRezBinding {
	uint32_t stages[kMVKShaderStageMax];
};

/** Represents a Vulkan pipeline layout. */
class MVKPipelineLayout : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_PIPELINE_LAYOUT; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT; }

	/** Binds descriptor sets to a command encoder. */
    void bindDescriptorSets(MVKCommandEncoder* cmdEncoder,
                            MVKVector<MVKDescriptorSet*>& descriptorSets,
                            uint32_t firstSet,
                            MVKVector<uint32_t>& dynamicOffsets);

	/** Updates a descriptor set in a command encoder. */
	void pushDescriptorSet(MVKCommandEncoder* cmdEncoder,
						   MVKVector<VkWriteDescriptorSet>& descriptorWrites,
						   uint32_t set);

	/** Updates a descriptor set from a template in a command encoder. */
	void pushDescriptorSet(MVKCommandEncoder* cmdEncoder,
						   MVKDescriptorUpdateTemplate* descriptorUpdateTemplate,
						   uint32_t set,
						   const void* pData);

	/** Populates the specified shader converter context. */
	void populateShaderConverterContext(SPIRVToMSLConversionConfiguration& context);

	/** Returns the current swizzle buffer bindings. */
	const MVKShaderImplicitRezBinding& getSwizzleBufferIndex() { return _swizzleBufferIndex; }

	/** Returns the current buffer size buffer bindings. */
	const MVKShaderImplicitRezBinding& getBufferSizeBufferIndex() { return _bufferSizeBufferIndex; }

	/** Returns the current indirect parameter buffer bindings. */
	const MVKShaderImplicitRezBinding& getIndirectParamsIndex() { return _indirectParamsIndex; }

	/** Returns the current captured output buffer bindings. */
	const MVKShaderImplicitRezBinding& getOutputBufferIndex() { return _outputBufferIndex; }

	/** Returns the current captured per-patch output buffer binding for the tess. control shader. */
	uint32_t getTessCtlPatchOutputBufferIndex() { return _tessCtlPatchOutputBufferIndex; }

	/** Returns the current tessellation level buffer binding for the tess. control shader. */
	uint32_t getTessCtlLevelBufferIndex() { return _tessCtlLevelBufferIndex; }

	/** Returns the number of textures in this layout. This is used to calculate the size of the swizzle buffer. */
	uint32_t getTextureCount() { return _pushConstantsMTLResourceIndexes.getMaxTextureIndex(); }

	/** Returns the number of buffers in this layout. This is used to calculate the size of the buffer size buffer. */
	uint32_t getBufferCount() { return _pushConstantsMTLResourceIndexes.getMaxBufferIndex(); }

	/** Returns the push constant binding info. */
	const MVKShaderResourceBinding& getPushConstantBindings() { return _pushConstantsMTLResourceIndexes; }

	/** Constructs an instance for the specified device. */
	MVKPipelineLayout(MVKDevice* device, const VkPipelineLayoutCreateInfo* pCreateInfo);

protected:
	void propogateDebugName() override {}

	MVKVectorInline<MVKDescriptorSetLayout, 8> _descriptorSetLayouts;
	MVKVectorInline<MVKShaderResourceBinding, 8> _dslMTLResourceIndexOffsets;
	MVKVectorInline<VkPushConstantRange, 8> _pushConstants;
	MVKShaderResourceBinding _pushConstantsMTLResourceIndexes;
	MVKShaderImplicitRezBinding _swizzleBufferIndex;
	MVKShaderImplicitRezBinding _bufferSizeBufferIndex;
	MVKShaderImplicitRezBinding _indirectParamsIndex;
	MVKShaderImplicitRezBinding _outputBufferIndex;
	uint32_t _tessCtlPatchOutputBufferIndex = 0;
	uint32_t _tessCtlLevelBufferIndex = 0;
};


#pragma mark -
#pragma mark MVKPipeline

static const uint32_t kMVKTessCtlInputBufferIndex = 30;
static const uint32_t kMVKTessCtlIndexBufferIndex = 29;
static const uint32_t kMVKTessCtlNumReservedBuffers = 2;

static const uint32_t kMVKTessEvalInputBufferIndex = 30;
static const uint32_t kMVKTessEvalPatchInputBufferIndex = 29;
static const uint32_t kMVKTessEvalLevelBufferIndex = 28;
static const uint32_t kMVKTessEvalNumReservedBuffers = 3;

/** Represents an abstract Vulkan pipeline. */
class MVKPipeline : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_PIPELINE; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT; }

	/** Returns the order of stages in this pipeline. Draws and dispatches must encode this pipeline once per stage. */
	virtual void getStages(MVKVector<uint32_t>& stages) = 0;

	/** Binds this pipeline to the specified command encoder. */
	virtual void encode(MVKCommandEncoder* cmdEncoder, uint32_t stage = 0) = 0;

	/** Binds the push constants to a command encoder. */
	void bindPushConstants(MVKCommandEncoder* cmdEncoder);

	/** Returns the current swizzle buffer bindings. */
	const MVKShaderImplicitRezBinding& getSwizzleBufferIndex() { return _swizzleBufferIndex; }

	/** Returns the current buffer size buffer bindings. */
	const MVKShaderImplicitRezBinding& getBufferSizeBufferIndex() { return _bufferSizeBufferIndex; }

	/** Returns whether or not full image view swizzling is enabled for this pipeline. */
	bool fullImageViewSwizzle() const { return _fullImageViewSwizzle; }

	/** Returns whether all internal Metal pipeline states are valid. */
	bool hasValidMTLPipelineStates() { return _hasValidMTLPipelineStates; }

	/** Constructs an instance for the device. layout, and parent (which may be NULL). */
	MVKPipeline(MVKDevice* device, MVKPipelineCache* pipelineCache, MVKPipelineLayout* layout, MVKPipeline* parent);

protected:
	void propogateDebugName() override {}

	MVKPipelineCache* _pipelineCache;
	MVKShaderImplicitRezBinding _swizzleBufferIndex;
	MVKShaderImplicitRezBinding _bufferSizeBufferIndex;
	MVKShaderResourceBinding _pushConstantsMTLResourceIndexes;
	bool _fullImageViewSwizzle;
	bool _hasValidMTLPipelineStates = true;

};


#pragma mark -
#pragma mark MVKGraphicsPipeline

/** Represents an Vulkan graphics pipeline. */
class MVKGraphicsPipeline : public MVKPipeline {

public:

	/** Returns the number and order of stages in this pipeline. Draws and dispatches must encode this pipeline once per stage. */
	void getStages(MVKVector<uint32_t>& stages) override;

	/** Binds this pipeline to the specified command encoder. */
	void encode(MVKCommandEncoder* cmdEncoder, uint32_t stage = 0) override;

    /** Returns whether this pipeline permits dynamic setting of the specifie state. */
    bool supportsDynamicState(VkDynamicState state);

    /** Returns whether this pipeline has tessellation shaders. */
    bool isTessellationPipeline() { return _pTessCtlSS && _pTessEvalSS && _tessInfo.patchControlPoints > 0; }

    /** Returns the number of input tessellation patch control points. */
    uint32_t getInputControlPointCount() { return _tessInfo.patchControlPoints; }

    /** Returns the number of output tessellation patch control points. */
    uint32_t getOutputControlPointCount() { return _outputControlPointCount; }

	/** Returns the current indirect parameter buffer bindings. */
	const MVKShaderImplicitRezBinding& getIndirectParamsIndex() { return _indirectParamsIndex; }

	/** Returns the current captured output buffer bindings. */
	const MVKShaderImplicitRezBinding& getOutputBufferIndex() { return _outputBufferIndex; }

	/** Returns the current captured per-patch output buffer binding for the tess. control shader. */
	uint32_t getTessCtlPatchOutputBufferIndex() { return _tessCtlPatchOutputBufferIndex; }

	/** Returns the current tessellation level buffer binding for the tess. control shader. */
	uint32_t getTessCtlLevelBufferIndex() { return _tessCtlLevelBufferIndex; }

	/** Returns true if the vertex shader needs a buffer to store its output. */
	bool needsVertexOutputBuffer() { return _needsVertexOutputBuffer; }

	/** Returns true if the tessellation control shader needs a buffer to store its per-vertex output. */
	bool needsTessCtlOutputBuffer() { return _needsTessCtlOutputBuffer; }

	/** Returns true if the tessellation control shader needs a buffer to store its per-patch output. */
	bool needsTessCtlPatchOutputBuffer() { return _needsTessCtlPatchOutputBuffer; }

	/** Constructs an instance for the device and parent (which may be NULL). */
	MVKGraphicsPipeline(MVKDevice* device,
						MVKPipelineCache* pipelineCache,
						MVKPipeline* parent,
						const VkGraphicsPipelineCreateInfo* pCreateInfo);

	~MVKGraphicsPipeline() override;

protected:
    id<MTLRenderPipelineState> getOrCompilePipeline(MTLRenderPipelineDescriptor* plDesc, id<MTLRenderPipelineState>& plState);
    id<MTLComputePipelineState> getOrCompilePipeline(MTLComputePipelineDescriptor* plDesc, id<MTLComputePipelineState>& plState, const char* compilerType);
    void initMTLRenderPipelineState(const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData);
    void initMVKShaderConverterContext(SPIRVToMSLConversionConfiguration& _shaderContext, const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData);
    void addVertexInputToShaderConverterContext(SPIRVToMSLConversionConfiguration& shaderContext, const VkGraphicsPipelineCreateInfo* pCreateInfo);
    void addPrevStageOutputToShaderConverterContext(SPIRVToMSLConversionConfiguration& shaderContext, std::vector<SPIRVShaderOutput>& outputs);
    MTLRenderPipelineDescriptor* getMTLRenderPipelineDescriptor(const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData);
    MTLRenderPipelineDescriptor* getMTLTessVertexStageDescriptor(const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData, SPIRVToMSLConversionConfiguration& shaderContext);
	MTLComputePipelineDescriptor* getMTLTessControlStageDescriptor(const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData, SPIRVToMSLConversionConfiguration& shaderContext);
	MTLRenderPipelineDescriptor* getMTLTessRasterStageDescriptor(const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData, SPIRVToMSLConversionConfiguration& shaderContext);
	bool addVertexShaderToPipeline(MTLRenderPipelineDescriptor* plDesc, const VkGraphicsPipelineCreateInfo* pCreateInfo, SPIRVToMSLConversionConfiguration& shaderContext);
	bool addTessCtlShaderToPipeline(MTLComputePipelineDescriptor* plDesc, const VkGraphicsPipelineCreateInfo* pCreateInfo, SPIRVToMSLConversionConfiguration& shaderContext, std::vector<SPIRVShaderOutput>& prevOutput);
	bool addTessEvalShaderToPipeline(MTLRenderPipelineDescriptor* plDesc, const VkGraphicsPipelineCreateInfo* pCreateInfo, SPIRVToMSLConversionConfiguration& shaderContext, std::vector<SPIRVShaderOutput>& prevOutput);
    bool addFragmentShaderToPipeline(MTLRenderPipelineDescriptor* plDesc, const VkGraphicsPipelineCreateInfo* pCreateInfo, SPIRVToMSLConversionConfiguration& shaderContext);
	bool addVertexInputToPipeline(MTLRenderPipelineDescriptor* plDesc, const VkPipelineVertexInputStateCreateInfo* pVI, const SPIRVToMSLConversionConfiguration& shaderContext);
    void addTessellationToPipeline(MTLRenderPipelineDescriptor* plDesc, const SPIRVTessReflectionData& reflectData, const VkPipelineTessellationStateCreateInfo* pTS);
    void addFragmentOutputToPipeline(MTLRenderPipelineDescriptor* plDesc, const SPIRVTessReflectionData& reflectData, const VkGraphicsPipelineCreateInfo* pCreateInfo, bool isTessellationVertexPipeline = false);
    bool isRenderingPoints(const VkGraphicsPipelineCreateInfo* pCreateInfo, const SPIRVTessReflectionData& reflectData);
	bool verifyImplicitBuffer(bool needsBuffer, MVKShaderImplicitRezBinding& index, MVKShaderStage stage, const char* name, uint32_t reservedBuffers);

	const VkPipelineShaderStageCreateInfo* _pVertexSS = nullptr;
	const VkPipelineShaderStageCreateInfo* _pTessCtlSS = nullptr;
	const VkPipelineShaderStageCreateInfo* _pTessEvalSS = nullptr;
	const VkPipelineShaderStageCreateInfo* _pFragmentSS = nullptr;

	VkPipelineTessellationStateCreateInfo _tessInfo;
	VkPipelineRasterizationStateCreateInfo _rasterInfo;
	VkPipelineDepthStencilStateCreateInfo _depthStencilInfo;

	MVKVectorInline<MTLViewport, kMVKCachedViewportScissorCount> _mtlViewports;
	MVKVectorInline<MTLScissorRect, kMVKCachedViewportScissorCount> _mtlScissors;

	MTLComputePipelineDescriptor* _mtlTessControlStageDesc = nil;

	id<MTLRenderPipelineState> _mtlTessVertexStageState = nil;
	id<MTLComputePipelineState> _mtlTessControlStageState = nil;
	id<MTLComputePipelineState> _mtlTessControlStageIndex16State = nil;
	id<MTLComputePipelineState> _mtlTessControlStageIndex32State = nil;
	id<MTLRenderPipelineState> _mtlPipelineState = nil;
	MTLCullMode _mtlCullMode;
	MTLWinding _mtlFrontWinding;
	MTLTriangleFillMode _mtlFillMode;
	MTLDepthClipMode _mtlDepthClipMode;
	MTLPrimitiveType _mtlPrimitiveType;

    float _blendConstants[4] = { 0.0, 0.0, 0.0, 1.0 };
    uint32_t _outputControlPointCount;
	MVKShaderImplicitRezBinding _indirectParamsIndex;
	MVKShaderImplicitRezBinding _outputBufferIndex;
	uint32_t _tessCtlPatchOutputBufferIndex = 0;
	uint32_t _tessCtlLevelBufferIndex = 0;

	bool _dynamicStateEnabled[VK_DYNAMIC_STATE_RANGE_SIZE];
	bool _hasDepthStencilInfo;
	bool _needsVertexSwizzleBuffer = false;
	bool _needsVertexBufferSizeBuffer = false;
	bool _needsVertexOutputBuffer = false;
	bool _needsTessCtlSwizzleBuffer = false;
	bool _needsTessCtlBufferSizeBuffer = false;
	bool _needsTessCtlOutputBuffer = false;
	bool _needsTessCtlPatchOutputBuffer = false;
	bool _needsTessCtlInput = false;
	bool _needsTessEvalSwizzleBuffer = false;
	bool _needsTessEvalBufferSizeBuffer = false;
	bool _needsFragmentSwizzleBuffer = false;
	bool _needsFragmentBufferSizeBuffer = false;
};


#pragma mark -
#pragma mark MVKComputePipeline

/** Represents an Vulkan compute pipeline. */
class MVKComputePipeline : public MVKPipeline {

public:

	/** Returns the number and order of stages in this pipeline. Draws and dispatches must encode this pipeline once per stage. */
	void getStages(MVKVector<uint32_t>& stages) override;

	/** Binds this pipeline to the specified command encoder. */
	void encode(MVKCommandEncoder* cmdEncoder, uint32_t = 0) override;

	/** Constructs an instance for the device and parent (which may be NULL). */
	MVKComputePipeline(MVKDevice* device,
					   MVKPipelineCache* pipelineCache,
					   MVKPipeline* parent,
					   const VkComputePipelineCreateInfo* pCreateInfo);

	~MVKComputePipeline() override;

protected:
    MVKMTLFunction getMTLFunction(const VkComputePipelineCreateInfo* pCreateInfo);

    id<MTLComputePipelineState> _mtlPipelineState;
    MTLSize _mtlThreadgroupSize;
    bool _needsSwizzleBuffer = false;
    bool _needsBufferSizeBuffer = false;
};


#pragma mark -
#pragma mark MVKPipelineCache

/** Represents a Vulkan pipeline cache. */
class MVKPipelineCache : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_PIPELINE_CACHE; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT; }

	/** 
	 * If pData is not null, serializes at most pDataSize bytes of the contents of the cache into that
	 * memory location, and returns the number of bytes serialized in pDataSize. If pData is null,
	 * returns the number of bytes required to serialize the contents of this pipeline cache.
	 */
	VkResult writeData(size_t* pDataSize, void* pData);

	/** Return a shader library from the specified shader context sourced from the specified shader module. */
	MVKShaderLibrary* getShaderLibrary(SPIRVToMSLConversionConfiguration* pContext, MVKShaderModule* shaderModule);

	/** Merges the contents of the specified number of pipeline caches into this cache. */
	VkResult mergePipelineCaches(uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches);

#pragma mark Construction

	/** Constructs an instance for the specified device. */
	MVKPipelineCache(MVKDevice* device, const VkPipelineCacheCreateInfo* pCreateInfo);

	~MVKPipelineCache() override;

protected:
	void propogateDebugName() override {}
	MVKShaderLibraryCache* getShaderLibraryCache(MVKShaderModuleKey smKey);
	void readData(const VkPipelineCacheCreateInfo* pCreateInfo);
	void writeData(std::ostream& outstream, bool isCounting = false);
	void markDirty();

	std::unordered_map<MVKShaderModuleKey, MVKShaderLibraryCache*> _shaderCache;
	size_t _dataSize = 0;
	std::mutex _shaderCacheLock;
};


#pragma mark -
#pragma mark MVKRenderPipelineCompiler

/**
 * Creates a MTLRenderPipelineState from a descriptor.
 *
 * Instances of this class are one-shot, and can only be used for a single pipeline compilation.
 */
class MVKRenderPipelineCompiler : public MVKMetalCompiler {

public:

	/**
	 * Returns a new (retained) MTLRenderPipelineState object compiled from the descriptor.
	 *
	 * If the Metal pipeline compiler does not return within MVKConfiguration::metalCompileTimeout
	 * nanoseconds, an error will be generated and logged, and nil will be returned.
	 */
	id<MTLRenderPipelineState> newMTLRenderPipelineState(MTLRenderPipelineDescriptor* mtlRPLDesc);


#pragma mark Construction

	MVKRenderPipelineCompiler(MVKVulkanAPIDeviceObject* owner) : MVKMetalCompiler(owner) {
		_compilerType = "Render pipeline";
		_pPerformanceTracker = &_owner->getDevice()->_performanceStatistics.shaderCompilation.pipelineCompile;
	}

	~MVKRenderPipelineCompiler() override;

protected:
	bool compileComplete(id<MTLRenderPipelineState> pipelineState, NSError *error);

	id<MTLRenderPipelineState> _mtlRenderPipelineState = nil;
};


#pragma mark -
#pragma mark MVKComputePipelineCompiler

/**
 * Creates a MTLComputePipelineState from a MTLFunction.
 *
 * Instances of this class are one-shot, and can only be used for a single pipeline compilation.
 */
class MVKComputePipelineCompiler : public MVKMetalCompiler {

public:

	/**
	 * Returns a new (retained) MTLComputePipelineState object compiled from the MTLFunction.
	 *
	 * If the Metal pipeline compiler does not return within MVKConfiguration::metalCompileTimeout
	 * nanoseconds, an error will be generated and logged, and nil will be returned.
	 */
	id<MTLComputePipelineState> newMTLComputePipelineState(id<MTLFunction> mtlFunction);

	/**
	 * Returns a new (retained) MTLComputePipelineState object compiled from the MTLComputePipelineDescriptor.
	 *
	 * If the Metal pipeline compiler does not return within MVKConfiguration::metalCompileTimeout
	 * nanoseconds, an error will be generated and logged, and nil will be returned.
	 */
	id<MTLComputePipelineState> newMTLComputePipelineState(MTLComputePipelineDescriptor* plDesc);


#pragma mark Construction

	MVKComputePipelineCompiler(MVKVulkanAPIDeviceObject* owner, const char* compilerType = nullptr) : MVKMetalCompiler(owner) {
		_compilerType = compilerType ? compilerType : "Compute pipeline";
		_pPerformanceTracker = &_owner->getDevice()->_performanceStatistics.shaderCompilation.pipelineCompile;
	}

	~MVKComputePipelineCompiler() override;

protected:
	bool compileComplete(id<MTLComputePipelineState> pipelineState, NSError *error);

	id<MTLComputePipelineState> _mtlComputePipelineState = nil;
};
