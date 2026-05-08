#if !defined(EINK_RA01S_GP02)
#include "Observer.h"
#include "configuration.h"
#else
#include "Observer.h"
#include "configuration.h"
void _observableLogNotify(const char *name, unsigned count)
{
    if (name)
        LOG_DEBUG("Observable[%s]: notifying %u observers", name, count);
}
void _observableLogResult(const char *name, int idx, int result)
{
    if (name)
        LOG_DEBUG("Observable[%s]: observer[%d] returned %d", name, idx, result);
}
void _observableLogRegister(const char *name, unsigned count)
{
    if (name)
        LOG_DEBUG("Observable[%s]: +observer registered (total %u)", name, count);
}
#endif // EINK_RA01S_GP02
