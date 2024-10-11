/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2023-2024 starfrost

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// cpuid.c: CPU Identification services (August 3, 2024)
#include <common/common.hpp>

#ifdef __GNUC__
#include <cpuid.hpp>
#endif

cvar_t* cpu_name;
cvar_t* cpu_vendor; // "AuthenticAMD", "GenuineIntel",...
cvar_t* cpu_features;
cvar_t* cpu_warning_dismissed;

#ifdef DEBUG
cvar_t* fake_defective_cpu;
#endif

#define CPUID_VENDOR_LENGTH	13	// Length of the CPUID vendor string, plus one for a null terminator
#define CPUID_NAME_LENGTH 0x31	// Length of the CPUID name/brand string (not available in some cases)

#define EAX	regs[0]
#define EBX regs[1]
#define ECX regs[2]
#define EDX regs[3]

//EAX=1, EDX

#define BIT_MMX (1 << 23)
#define BIT_SSE (1 << 25)
#define BIT_SSE2 (1 << 26)

//EAX=1, ECX

#define BIT_SSE3 1
#define BIT_SSSE3 (1 << 9)
#define BIT_FMA3 (1 << 12)
#define BIT_SSE41 (1 << 19)
#define BIT_SSE42 (1 << 20)
#define BIT_AVX1 (1 << 28)

//EAX=7h, EBX

#define BIT_AVX2 (1 << 5)

//EAX=80000001h, EDX

#define BIT_3DNOW (1 << 31)

//EAX=80000001h, ECX 

#define BIT_SSE4A (1 << 6)

const char* cpu_known_defective_warning_title = "[STRING_WARNING_CPU_DEFECTIVE_TITLE]";
const char* cpu_known_defective_warning_description =
"[STRING_WARNING_CPU_DEFECTIVE_DESCRIPTION]";

void CPUID_Init()
{
	int32_t regs[4] = { 0x00 };
	uint32_t highest_basic_leaf = 0;
	uint32_t highest_extended_leaf = 0;

	char cpu_vendor_string[CPUID_VENDOR_LENGTH] = { 0 };
	char cpu_name_string[CPUID_NAME_LENGTH] = { 0 };

	// initialise the cvars
	cpu_name = Cvar_Get("cpu_name", "Intel Pentium 5 Tejas @ 7.4Ghz, 10000W TDP", CVAR_NOSET);
	cpu_vendor = Cvar_Get("cpu_vendor", "S3 Graphics", CVAR_NOSET);
	cpu_features = Cvar_Get("cpu_features", "69696969", CVAR_NOSET);
	cpu_warning_dismissed = Cvar_Get("cpu_warning_dismissed", "0", CVAR_ARCHIVE);

#ifdef DEBUG
	fake_defective_cpu = Cvar_Get("fake_defective_cpu", "0", 0);
#endif
	Com_Printf("CPUID_Init: CPU Identification:\n");

#ifdef _MSC_VER
	// Get CPU Brand String
	__cpuidex(regs, 0x0, 0x0);
#elif __GNUC__

#endif

	highest_basic_leaf = EAX;

	strncpy(cpu_vendor_string, (const char*)&EBX, 4);
	strncpy(cpu_vendor_string + 4, (const char*)&EDX, 4);
	strncpy(cpu_vendor_string + 8, (const char*)&ECX, 4);

	// yes, some of these have been recorded, most likely a manufacturing defect
	if (!strcmp(cpu_vendor_string, "GenuineIotel"))
		strncpy(cpu_vendor_string, "GenuineIntel", CPUID_VENDOR_LENGTH); // changed from overwriting a byte for C++ conversion due to its stricter type system

	// might change this to an enum but idk
	Cvar_ForceSet("cpu_vendor", cpu_vendor_string);

	// get supported extended cpuid leaf
#ifdef _MSC_VER
	__cpuidex(regs, 0x80000000, 0x0);
#elif __GNUC__
#endif

	highest_extended_leaf = EAX;

	if (highest_extended_leaf >= 0x80000004)
	{
		uint32_t position = 0x00;

		// Brand string is leaves 0x80000002-0x80000004
		for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++)
		{
			// yes the order is different
			__cpuidex(regs, leaf, 0x0);

			strncpy(cpu_name_string + position, (const char*)&EAX, 4);
			position += 0x04;
			strncpy(cpu_name_string + position, (const char*)&EBX, 4);
			position += 0x04;
			strncpy(cpu_name_string + position, (const char*)&ECX, 4);
			position += 0x04;
			strncpy(cpu_name_string + position, (const char*)&EDX, 4);
			position += 0x04;
		}

		// cpu name is right-padded so unpad it
		char* ptr = cpu_name_string + strlen(cpu_name_string) - 1; // already null-terminated

		// prevent overflow
		if (ptr > cpu_name_string + CPUID_NAME_LENGTH - 1)
			ptr = cpu_name_string + CPUID_NAME_LENGTH - 1;

		char* ptr_end = ptr;

		while (ptr >= cpu_name_string
			&& isspace(*ptr))
		{
			ptr--;
		}

		// if there is any rightpadding, pad it
		if (ptr != ptr_end)
		{
			uint32_t position = ptr - cpu_name_string + 1; //+1 as we are pointing at the last character

			cpu_name_string[position] = '\0';
		}

		Cvar_ForceSet("cpu_name", cpu_name_string);
	}
	else
	{
		Cvar_ForceSet("cpu_name", cpu_vendor->string);
	}

	// Feature identification

	int32_t features = 0;

	// first, identify basic features
	if (highest_basic_leaf >= 1)
	{
		__cpuidex(regs, 0x1, 0x0);

		if (EDX & BIT_MMX)
			features |= cpu_feature_mmx;

		if (EDX & BIT_SSE)
			features |= cpu_feature_sse1;

		if (EDX & BIT_SSE2)
			features |= cpu_feature_sse2;

		if (ECX & BIT_SSE3)
			features |= cpu_feature_sse3;

		if (ECX & BIT_SSSE3)
			features |= cpu_feature_ssse3;
		
		if (ECX & BIT_FMA3)
			features |= cpu_feature_fma3;

		if (ECX & BIT_SSE41)
			features |= cpu_feature_sse41;

		if (ECX & BIT_SSE42)
			features |= cpu_feature_sse42;

		if (ECX & BIT_AVX1)
			features |= cpu_feature_avx1;
	}

	// Extended features
	if (highest_basic_leaf >= 7)
	{
		__cpuidex(regs, 0x7, 0x0);

		if (EBX & BIT_AVX2)
			features |= cpu_feature_avx2;
	}

	if (highest_extended_leaf >= 0x80000001)
	{
		__cpuidex(regs, 0x80000001, 0x0);

		if (EDX & BIT_3DNOW)
			features |= cpu_feature_3dnow;

		if (EBX & BIT_SSE4A)
			features |= cpu_feature_sse4a;
	}

	// we have to use Cvar_ForceSet as it's CVAR_NOSET and just setting the value doesn't fully update the state
	char str_buf[9] = { 0 }; // uint32_t + 1 for margin

	snprintf(str_buf, 9, "%d", features);

	Cvar_ForceSet("cpu_features", str_buf);

	Com_Printf("CPUID_Init: %s (%s, Feature Flags: 0x%2x)\n", cpu_name->string, cpu_vendor->string, (uint32_t)cpu_features->value);

	if (CPUID_IsDefectiveIntelCPU()
		&& !cpu_warning_dismissed->value)
	{
		Com_Printf("CPUID_Init: WARNING: Intel CPU with high failure rates found\n");
		Sys_Msgbox(cpu_known_defective_warning_title, 0x30, cpu_known_defective_warning_description); // warning box

		// make sure the warning is only displayed once
		Cvar_SetValue("cpu_warning_dismissed", 1.0f);
	}
}

bool CPUID_IsDefectiveIntelCPU()
{
	// Fuzzy match any Intel Core 13700/13900/14700/14900 to account for the multipicity of CPUIDs rather than using the family name.
	// x600s are not 65w cpus and are not affected as far as we know, but x600Ks are and do appear to be failing

#if DEBUG
	if (fake_defective_cpu->value)
		return true; 
#endif

	if (!strcmp(cpu_vendor->string, "GenuineIntel")
		&& strstr(cpu_name->string, "Core"))
	{
		return (strstr(cpu_name->string, "13900")
			|| strstr(cpu_name->string, "13700")
			|| strstr(cpu_name->string, "13600K")
			|| strstr(cpu_name->string, "14900")
			|| strstr(cpu_name->string, "14700")
			|| strstr(cpu_name->string, "14600K"));
	}
	else
	{
		return false;
	}
}