#pragma once
#include "common.h"
#include "COCLDevice.h"

class CProfiler {

	public:

		struct ProfiledElement {
			std::string name;
			LARGE_INTEGER frequency;
			LARGE_INTEGER start;
			LARGE_INTEGER end;
			__int64 totalTicks;
			double totalTime;
			bool isStarted = false;
		};

		// Application return codes
		enum profilerFlags {
			START_PROFILING = 0,
			END_PROFILING = 1
		};


		CProfiler();
		CProfiler(bool);
		~CProfiler(void);
		void profile(std::string, int, COCLDevice* device = nullptr);
		bool doesntExists(std::string);
		void createProfileElement(std::string);
		void logValues();
		CProfiler::ProfiledElement* getProfileElement(std::string);
	private:
		std::vector<ProfiledElement*> profiledElements;
		bool activated = false;



};