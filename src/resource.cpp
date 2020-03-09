/*
MIT License

Copyright (c) 2020 Silverlan
Copyright (c) 2015 Steam Database

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "source2/resource.hpp"
#include "source2/resource_data.hpp"
#include "source2/resource_edit_info.hpp"
#include "source2/redi/redi.hpp"
#include <algorithm>
#include <array>
#include <sharedutils/util_string.h>
#include <fsys/filesystem.h>

using namespace source2;

#pragma optimize("",off)
resource::Block *resource::Resource::FindBlock(BlockType type)
{
	auto it = std::find_if(m_blocks.begin(),m_blocks.end(),[type](const std::shared_ptr<Block> &block) {
		return block->GetType() == type;
		});
	return (it != m_blocks.end()) ? it->get() : nullptr;
}
const resource::Block *resource::Resource::FindBlock(BlockType type) const {return const_cast<Resource*>(this)->FindBlock(type);}
const std::vector<std::shared_ptr<resource::Block>> &resource::Resource::GetBlocks() const {return m_blocks;}
void resource::Resource::Read(std::shared_ptr<VFilePtrInternal> f)
{
	auto fileSize = f->Read<uint32_t>();
	if (fileSize == 0x55AA1234)
		throw std::runtime_error{"Use ValvePak library to parse VPK files."};
	if (fileSize == 0x32736376) // "vcs2"
		throw std::runtime_error{"Use CompiledShader() class to parse compiled shader files."};

	auto headerVersion = f->Read<uint16_t>();
	constexpr uint32_t knownHeaderVersion = 12u;
	if (headerVersion != knownHeaderVersion)
		throw std::runtime_error{"(Bad header version. (" +std::to_string(headerVersion) +" != expected " +std::to_string(knownHeaderVersion) +")"};

	m_version = f->Read<uint16_t>();
	auto startOffset = f->Tell();
	auto blockOffset = f->Read<uint32_t>();
	auto blockCount = f->Read<uint32_t>();
	m_blocks.reserve(blockCount);
	for(auto i=decltype(blockCount){0u};i<blockCount;++i)
	{
		auto blockType = f->Read<std::array<char,4>>(); // TODO: UTF8?
		auto position = f->Tell();
		auto offset = position +f->Read<uint32_t>();
		auto size = f->Read<uint32_t>();
		std::shared_ptr<source2::resource::Block> block = nullptr;
		if(ustring::compare(blockType.data(),"DATA",true,blockType.size()))
		{
			f->Seek(offset);
			auto magic = f->Read<uint32_t>();
			if (magic == source2::resource::BinaryKV3::MAGIC || magic == source2::resource::BinaryKV3::MAGIC2)
				block = std::make_shared<source2::resource::BinaryKV3>();
			f->Seek(position);
		}
		if(block == nullptr)
			block = ConstructFromType(std::string{blockType.data(),blockType.size()});

		block->SetOffset(offset);
		block->SetSize(size);
		block->Read(*this,f);

		m_blocks.push_back(block);

		switch(block->GetType())
		{
		case BlockType::REDI:
			// Try to determine resource type by looking at first compiler indentifier
			if(m_resourceType == ResourceType::Unknown)
			{
				auto &specialDeps = static_cast<SpecialDependencies&>(static_cast<ResourceEditInfo&>(*block).GetStruct(REDIStruct::SpecialDependencies));
				auto &list = specialDeps.GetSpecialDependencies();
				if(list.empty() == false)
					m_resourceType = SpecialDependencies::DetermineResourceTypeByCompilerIdentifier(list.front());
			}
			break;
		case BlockType::NTRO:
			if(m_resourceType == ResourceType::Unknown)
			{
				auto *manifest = GetIntrospectionManifest();
				if(manifest && manifest->GetReferencedStructs().size() > 0)
				{
					auto &strct = manifest->GetReferencedStructs().front();
					if(strct.name == "VSoundEventScript_t")
						m_resourceType = ResourceType::SoundEventScript;
					else if(strct.name == "CWorldVisibility")
						m_resourceType = ResourceType::WorldVisibility;
				}
			}
			break;
		}

		f->Seek(position +sizeof(uint32_t) *2);
	}
}
std::shared_ptr<source2::resource::Block> resource::Resource::ConstructFromType(std::string input)
{
	if(input == "DATA")
		return ConstructResourceType();
	else if(input == "REDI")
		return std::make_shared<source2::resource::ResourceEditInfo>();
	else if(input == "RERL")
		return std::make_shared<source2::resource::ResourceExtRefList>();
	else if(input == "VBIB")
		return std::make_shared<source2::resource::VBIB>();
	else if(input == "NTRO")
		return std::make_shared<source2::resource::ResourceIntrospectionManifest>();
	else if(input == "VXVS")
		return std::make_shared<source2::resource::VXVS>();
	else if(input == "SNAP")
		return std::make_shared<source2::resource::SNAP>();
	else if(input == "MBUF")
		return std::make_shared<source2::resource::MBUF>();
	else if(input == "CTRL")
		return std::make_shared<BinaryKV3>(BlockType::CTRL);
	else if(input == "MDAT")
		return std::make_shared<BinaryKV3>(BlockType::MDAT);
	else if(input == "MRPH")
		return std::make_shared<BinaryKV3>(BlockType::MRPH);
	else if(input == "ANIM")
		return std::make_shared<BinaryKV3>(BlockType::ANIM);
	else if(input == "ASEQ")
		return std::make_shared<BinaryKV3>(BlockType::ASEQ);
	else if(input == "AGRP")
		return std::make_shared<BinaryKV3>(BlockType::AGRP);
	else if(input == "PHYS")
		return std::make_shared<BinaryKV3>(BlockType::PHYS);
	throw std::runtime_error{"Support for type " +input +" not yet implemented!"};
}
uint32_t resource::Resource::GetVersion() const {return m_version;}
std::shared_ptr<source2::resource::ResourceData> resource::Resource::ConstructResourceType()
{
	switch (m_resourceType)
	{
	case ResourceType::Panorama:
	case ResourceType::PanoramaStyle:
	case ResourceType::PanoramaScript:
	case ResourceType::PanoramaLayout:
	case ResourceType::PanoramaDynamicImages:
	case ResourceType::PanoramaVectorGraphic:
		return std::make_shared<source2::resource::Panorama>();

	case ResourceType::Sound:
		return std::make_shared<source2::resource::Sound>();

	case ResourceType::Texture:
		return std::make_shared<source2::resource::Texture>();

	case ResourceType::Material:
	    return std::make_shared<source2::resource::Material>();

	case ResourceType::SoundEventScript:
		return std::make_shared<source2::resource::SoundEventScript>();

	case ResourceType::Particle:
		return std::make_shared<source2::resource::BinaryKV3>();

	case ResourceType::Mesh:
		if(m_version == 0)
			break;
		return std::make_shared<source2::resource::BinaryKV3>();
	}

	if(GetIntrospectionManifest())
		return std::make_shared<source2::resource::NTRO>();
	return std::make_shared<source2::resource::ResourceData>();
}
resource::ResourceIntrospectionManifest *resource::Resource::GetIntrospectionManifest()
{
	return static_cast<resource::ResourceIntrospectionManifest*>(FindBlock(BlockType::NTRO));
}
const resource::ResourceIntrospectionManifest *resource::Resource::GetIntrospectionManifest() const {return const_cast<Resource*>(this)->GetIntrospectionManifest();}

resource::ResourceExtRefList *resource::Resource::GetExternalReferences()
{
	return static_cast<resource::ResourceExtRefList*>(FindBlock(BlockType::RERL));
}
const resource::ResourceExtRefList *resource::Resource::GetExternalReferences() const {return const_cast<Resource*>(this)->GetExternalReferences();}
#pragma optimize("",on)
