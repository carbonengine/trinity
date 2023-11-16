#pragma once

enum Tr2ALMemoryType
{
	AL_MEMORY_VIDEO = 1 << 0,  // resources created on video card memory
	AL_MEMORY_MANAGED = 1 << 1, // resources created in device memory
};

typedef std::map<std::string, std::string> Tr2DeviceResourceDescriptionAL;
typedef void ( *Tr2DescribeDeviceResourceOperationAL )( Tr2ALMemoryType memoryType, const Tr2DeviceResourceDescriptionAL& description );

typedef int Tr2ALMemoryTypes;


namespace TrinityALImpl
{
	
	class Tr2BaseDeviceResourceAL
	{
	public:
		typedef void ( *ResourceOperation )( Tr2BaseDeviceResourceAL* );

		Tr2BaseDeviceResourceAL();
		virtual ~Tr2BaseDeviceResourceAL();

		virtual bool IsResourceValid() const = 0;
		virtual Tr2ALMemoryType GetResourceMemoryClass() const = 0;
		virtual void Destroy() = 0;
		virtual void Describe( Tr2DeviceResourceDescriptionAL& description ) const = 0;

		static void EnumerateResources( ResourceOperation* operation );
	};

	template <typename T>
	class Tr2DeviceResourceAL: public Tr2BaseDeviceResourceAL
	{
	public:
		bool IsResourceValid() const override
		{
			return static_cast<const T*>( this )->IsValid();
		}

		Tr2ALMemoryType GetResourceMemoryClass() const override
		{
			return static_cast<const T*>( this )->GetMemoryClass();
		}
	};

}


void DescribeDeviceResources( Tr2DescribeDeviceResourceOperationAL operation );
void DestroyDeviceResources( Tr2ALMemoryTypes memoryTypes );
