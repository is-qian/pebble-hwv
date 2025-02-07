#ifndef HWV_DRIVERS_DRV2604_H_
#define HWV_DRIVERS_DRV2604_H_

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup drivers_haptic Haptic drivers
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup drivers_haptic_ops Haptic driver operations
 * @{
 */

/** @brief Haptic driver class operations */
__subsystem struct haptic_driver_api {
	/**
	 * @brief Turn the haptic device on.
	 *
	 * @param dev Haptic device instance.
	 * @param ampl Amplitude (0-100).
	 *
	 * @retval 0 if successful.
	 * @retval -errno Other negative errno code on failure.
	 */
	int (*configure)(const struct device *dev, uint8_t ampl);
};

/** @} */

/**
 * @defgroup drivers_haptic_api Haptic driver API
 * @{
 */

/**
 * @brief Configure haptic.
 *
 * @param dev Haptic device instance.
 * @param ampl Amplitude (0-100).
 *
 * @retval 0 if successful.
 * @retval -EINVAL If the amplitude is greater than 100.
 * @retval -errno Other negative errno code on failure.
 */
__syscall int haptic_configure(const struct device *dev, uint8_t ampl);

static inline int z_impl_haptic_configure(const struct device *dev, uint8_t ampl)
{
	__ASSERT_NO_MSG(DEVICE_API_IS(haptic, dev));

	if (ampl > 100U) {
		return -EINVAL;
	}

	return DEVICE_API_GET(haptic, dev)->configure(dev, ampl);
}

#include <syscalls/haptic.h>

/** @} */

/** @} */

#endif /* APP_DRIVERS_BLINK_H_ */
