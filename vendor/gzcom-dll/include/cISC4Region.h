#pragma once
#include "cIGZUnknown.h"
#include "cRZAutoRefCount.h"
#include "EASTLConfigSC4.h"
#include "EASTL\vector.h"
#include <EASTL/list.h>

class cIGZString;
class cLocation;
class cISC4RegionalCity;


class cISC4Region : public cIGZUnknown
{
	public:
		virtual char* GetName(void) = 0;
		virtual bool SetName(const cIGZString& szName) = 0;

		virtual char* GetDirectoryName(void) = 0;
		virtual bool SetDirectoryName(const cIGZString& szName) = 0;

		virtual bool LoadConfig(void) = 0;

		virtual bool Init(void) = 0;
		virtual bool Shutdown(void) = 0;
		virtual bool Delete(void) = 0;

		virtual cISC4RegionalCity* GetCity(uint32_t x, uint32_t y) = 0;
		virtual cISC4RegionalCity**& InsertCity(cISC4RegionalCity* pCity) = 0;
		virtual bool RemoveCity(cISC4RegionalCity*& pCity) = 0;
		virtual bool DeleteCity(cISC4RegionalCity*& pCity) = 0;
		virtual bool ReloadCity(cISC4RegionalCity*& pCity) = 0;
		virtual bool MoveCity(cISC4Region* pRegion, cISC4RegionalCity* pCity, int32_t x, int32_t y) = 0;
		virtual bool GetAllCities(eastl::list<cRZAutoRefCount<cISC4RegionalCity>>& pList) = 0;

		virtual int GetBaseTerrainType(void) = 0;
		virtual cISC4Region* SetBaseTerrainType(int nType) = 0;

		virtual int GetBaseTerrainHeight(void) = 0;
		virtual int32_t GetWaterPrefs(uint8_t& cUnknown1, uint8_t& cUnknown2) = 0;

		virtual bool ResetTutorialCity(uint32_t dwTutorialCityID) = 0;

		virtual void GetCityLocations(eastl::vector<cLocation> pVector) = 0;
		virtual int32_t GetBoundingRect(intptr_t pRectLongs) = 0;
};