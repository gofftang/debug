/*
 * Winrise HBS1632 LED driver
 *
 * Copyright © 2016 Zowee Technologies Co., Ltd.
 * 	Gofftang <gofftagn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#endif

#include <linux/fb.h>
#include <linux/mfd/hbs1632.h>

#include <asm/uaccess.h>
#include "hbs1632-fb.h"

#define TE_PERIOD 		(60 * HZ)

struct hbs1632_device {
	struct device *dev;
	struct fb_info *info;
	struct hbs1632_chip *chip;

	void __iomem *video_mem;
	unsigned long video_mem_phys;
	size_t video_mem_size;

	int brightness;
	int blink;

	struct timer_list te_timer;
};

#define to_smem_len(info) (info->var.xres_virtual * info->var.yres_virtual / 8)

static struct hbs1632_device *to_hbs1632_device(struct fb_info *info)
{
	return container_of((void *)info, struct hbs1632_device, info);
}

static int hbs1632_write(struct hbs1632_chip *chip, const u8 *data, u8 len)
{
	if (!chip && !chip->cl)
		return -EFAULT;

    return hbs1632_i2c_write(chip->cl, data, len);
}

static int hbs1632_set_brightness(struct hbs1632_chip *chip, u8 duty)
{
	if (duty >= HBS1632_PWM_DUTY_TOP) {
		dev_err(chip->dev, "invalid duty: %u\n", duty);
		return -EINVAL;
	}

	duty = HBS1632_CMD_PWM_DUTY(duty);
	return hbs1632_write(chip, &duty, 1);
}

static int hbs1632_set_blink(struct hbs1632_chip *chip, u8 blink_type)
{
	if (blink_type >= HBS1632_BLINK_TYPE_TOP) {
		dev_err(chip->dev, "invalid blink type: %u\n", blink_type);
		return -EINVAL;
	}

	blink_type = HBS1632_CMD_BLINK_BASE + blink_type;
	return hbs1632_write(chip, &blink_type, 1);
}

static int hbs1632_start_engine(struct hbs1632_chip *chip)
{
	u8 cmd_set[] = {
		HBS1632_CMD_SYS_DIS,
		HBS1632_CMD_COM_OPT(chip->com_code),
		HBS1632_CMD_SYS_EN,
		HBS1632_CMD_LED_ON
	};

    return hbs1632_write(chip, cmd_set, sizeof(cmd_set));
}

static int hbs1632_easy_flush(struct hbs1632_device *hdev)
{
#define TEMP_SIZE	128
	u8 temp[TEMP_SIZE+1] = {0};
	u8 size = to_smem_len(hdev->info);

	size = size > TEMP_SIZE ? TEMP_SIZE : size;
	/* temp[0] fixed 0x00 */
	memcpy(temp+1, hdev->video_mem, size);

	return hbs1632_write(hdev->chip, temp, size+1);
}

static struct fb_fix_screeninfo hbs1632_default_fix __initdata = {
	.id         = "hbs1632",
	.type       = FB_TYPE_PACKED_PIXELS,
	.ypanstep   = 1,
};

static struct fb_var_screeninfo hbs1632_default_var __initdata = {
	.xres           = 16,
	.yres           = 16,
	.xres_virtual   = 16,
	.yres_virtual   = 16,
	.bits_per_pixel = 1,
};

static void hbs1632_te_callback(unsigned long arg)
{
	struct hbs1632_device *hdev = (struct hbs1632_device *)arg;

	dev_dbg(hdev->dev, "TE timer\n");

	hbs1632_easy_flush(hdev);

    mod_timer(&hdev->te_timer, jiffies + TE_PERIOD);
}

static int hbs1632_fb_check_var(struct fb_var_screeninfo *var,
			struct fb_info *info)
{
	if (var->bits_per_pixel != 1)
		return -EINVAL;

	return 0;
}

static int hbs1632_fb_pan_display(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	struct hbs1632_device *hdev = to_hbs1632_device(info);

	hbs1632_easy_flush(hdev);
	return 0;
}

static int hbs1632_fb_ioctl(struct fb_info *info, unsigned int cmd,
			  unsigned long arg)
{
	struct hbs1632_device *hdev = to_hbs1632_device(info);

	switch (cmd) {
	case FBIOGET_BRIGHTNESS:
		if (copy_to_user((void __user *)arg, (const void *)&hdev->brightness, sizeof(int)))
			return -EFAULT;
		break;

	case FBIOPUT_BRIGHTNESS: {
			int brightness;

			if (copy_from_user(&brightness, (const void __user *)arg, sizeof(int)))
				return -EFAULT;

			if (hbs1632_set_brightness(hdev->chip, (u8)brightness))
				return -EFAULT;
		}
		break;

	case FBIOGET_BLINK:
		if (copy_to_user((void __user *)arg, (const void *)&hdev->blink, sizeof(int)))
			return -EFAULT;
		break;

	case FBIOPUT_BLINK: {
			int blink;

			if (copy_from_user(&blink, (const void __user *)arg, sizeof(int)))
				return -EFAULT;

			if (hbs1632_set_blink(hdev->chip, (u8)blink))
				return -EFAULT;
		}
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int hbs1632_fb_blank(int blank, struct fb_info *info)
{
	struct hbs1632_device *hdev = to_hbs1632_device(info);
	int smem_len = to_smem_len(hdev->info);
	u8 *smem_start = (u8 *)hdev->video_mem;
	u8 val = blank ? 0xFF : 0x00;

	memset(smem_start, val, smem_len);
	hbs1632_easy_flush(hdev);
	return 0;
}

static struct fb_ops hbs1632_fb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = hbs1632_fb_check_var,
	.fb_pan_display = hbs1632_fb_pan_display,
	.fb_ioctl = hbs1632_fb_ioctl,
	// .fb_fillrect = cfb_fillrect,
	// .fb_copyarea = cfb_copyarea,
	// .fb_imageblit = cfb_imageblit,
	.fb_blank = hbs1632_fb_blank,
};

static struct hbs1632_chip *hbs1632_chip_alloc(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct hbs1632_chip *chip;
	int sda_gpio, scl_gpio, interval = 0;
	enum of_gpio_flags flag;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(dev, "hbs1632_chip alloc error\n");
		return NULL;
	}
	memset(chip, 0, sizeof(*chip));

#ifdef CONFIG_OF
	scl_gpio = of_get_named_gpio_flags(node, "i2c-gpio,scl", 0, &flag);
	if (!gpio_is_valid(scl_gpio)) {
		dev_err(dev, "scl_gpio: invalid gpio : %d\n",scl_gpio);
		goto free_chip;
	}

	sda_gpio = of_get_named_gpio_flags(node, "i2c-gpio,sda", 0, &flag);
	if (!gpio_is_valid(sda_gpio)) {
		dev_err(dev, "sda_gpio: invalid gpio : %d\n",sda_gpio);
		goto free_chip;
	}

	of_property_read_u32(node, "com-number", &chip->nrcoms);
	of_property_read_u32(node, "row-number", &chip->nrrows);
	of_property_read_u32(node, "com-option", &chip->com_code);
	of_property_read_u32(node, "i2c-gpio,delay-us", &interval);

	chip->cl = hbs1632_i2c_create(scl_gpio, sda_gpio, interval);
	if (!chip->cl) {
		dev_err(dev, "i2c_create err\n");
		goto free_chip;
	}
#endif

	chip->dev = dev;
	return chip;

#ifdef CONFIG_OF
free_chip:
	devm_kfree(dev, chip);
	return NULL;
#endif
}

void hbs1632_chip_release(struct hbs1632_chip *chip)
{
#ifdef CONFIG_OF
	hbs1632_i2c_destroy(chip->cl);
#endif
	devm_kfree(chip->dev, chip);
}

static struct hbs1632_device *hbs1632_device_alloc(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hbs1632_device *hdev;
	struct hbs1632_chip *chip;
	struct fb_info *info;

	info = framebuffer_alloc(0, dev);
	if (!info) {
		dev_err(dev, "framebuffer_alloc error\n");
		return NULL;
	}
	info->flags = FBINFO_DEFAULT;
	info->fix = hbs1632_default_fix;
	info->var = hbs1632_default_var;
	info->fbops = &hbs1632_fb_ops;

	chip = hbs1632_chip_alloc(pdev);
	if (!chip)
		goto release_info;

	hdev = devm_kzalloc(dev, sizeof(*hdev), GFP_KERNEL);
	if (!hdev) {
		dev_err(dev, "hbs1632_device alloc error\n");
		goto release_chip;
	}
	memset(hdev, 0, sizeof(*hdev));

	hdev->video_mem_size = PAGE_ALIGN(to_smem_len(info));
	hdev->video_mem = alloc_pages_exact(hdev->video_mem_size, GFP_KERNEL | __GFP_ZERO);
	if (!hdev->video_mem) {
		dev_err(dev, "video_mem alloc error\n");
		goto free_dev;
	}
	hdev->video_mem_phys = virt_to_phys(hdev->video_mem);
	info->fix.smem_start = hdev->video_mem_phys;
	info->fix.smem_len = hdev->video_mem_size;

	init_timer(&hdev->te_timer);
	hdev->te_timer.function = hbs1632_te_callback;
	hdev->te_timer.data = (unsigned long)hdev;

	hdev->chip = chip;
	hdev->info = info;
	hdev->dev = dev;
	return hdev;

free_dev:
	devm_kfree(dev, hdev);
release_chip:
	hbs1632_chip_release(chip);
release_info:
	framebuffer_release(info);
	return NULL;
}

static void hbs1632_device_release(struct hbs1632_device *hdev)
{
	hbs1632_chip_release(hdev->chip);
	framebuffer_release(hdev->info);
	free_pages_exact(hdev->video_mem, hdev->video_mem_size);
	devm_kfree(hdev->dev, hdev);
}

static int hbs1632_fb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct hbs1632_device *hdev;

	dev_info(dev, "%s\n", __FUNCTION__);

	hdev = hbs1632_device_alloc(pdev);
	if (!hdev)
		return -ENOMEM;
	platform_set_drvdata(pdev, hdev);

	hbs1632_start_engine(hdev->chip);
#ifdef CONFIG_OF
	{
		const char *state;

		state = of_get_property(node, "default-state", NULL);
		if (state && strcmp(state, "on") == 0)
			hbs1632_fb_blank(1, hdev->info);
	}
#endif

	mod_timer(&hdev->te_timer, jiffies + TE_PERIOD);
	return 0;
}

static int hbs1632_fb_remove(struct platform_device *pdev)
{
	struct hbs1632_device *hdev = platform_get_drvdata(pdev);

	hbs1632_device_release(hdev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_match_hbs1632_fb[] = {
	{ .compatible = "winrise,hbs1632"},
	{},
};
#endif

static struct platform_driver hbs1632_fb_driver = {
	.driver = {
		.name	= "hbs1632",
#ifdef CONFIG_OF
		.of_match_table = of_match_hbs1632_fb,
#endif
	},
	.probe		= hbs1632_fb_probe,
	.remove		= hbs1632_fb_remove,
};

module_platform_driver(hbs1632_fb_driver);

MODULE_DESCRIPTION("Winrise HBS1632 LED FB Driver");
MODULE_AUTHOR("Gofftang <gofftagn@gmail.com>");
MODULE_LICENSE("GPL");
