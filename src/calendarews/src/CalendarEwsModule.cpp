#include "mozilla/ModuleUtils.h"

#include "nsServiceManagerUtils.h"
#include "nsILocalFile.h"
#include "nsComponentManagerUtils.h"

#include "CalEwsCID.h"
#include "CalEwsCalendar.h"

#ifdef _WIN32
#include <windows.h>
#endif

NS_DEFINE_NAMED_CID(CAL_EWS_CALENDAR_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(CalEwsCalendar);

static const mozilla::Module::CIDEntry kCalendarEwsCIDs[] = {
  { &kCAL_EWS_CALENDAR_CID, false, nullptr, CalEwsCalendarConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kCalendarEwsContracts[] = {
  { CAL_EWS_CALENDAR_CONTRACTID, &kCAL_EWS_CALENDAR_CID },
  { nullptr }
};

static nsresult ModuleLoadFunc() {
	nsresult rv = NS_OK;
	return rv;
}

static const mozilla::Module kCalendarEwsModule = {
  mozilla::Module::kVersion,
  kCalendarEwsCIDs,
  kCalendarEwsContracts,
  nullptr,
  nullptr,
  ModuleLoadFunc,
  nullptr
};

NSMODULE_DEFN(nsCalendarEwsModule) = &kCalendarEwsModule;

NS_IMPL_MOZILLA192_NSGETMODULE(&kCalendarEwsModule)

