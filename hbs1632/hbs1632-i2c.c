/*
 * LEDs driver for GPIOs
 *
 * Copyright (C) 2007 8D Technologies inc.
 * Raphael Assenat <raph@8d.com>
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mfd/hbs1632.h>

// #define DEBUG

struct hbs1632_i2c *hbs1632_i2c_create(int scl, int sda, int interval)
{
	struct hbs1632_i2c *i2c;
	int ret;

	i2c = kmalloc(sizeof(struct hbs1632_i2c), GFP_KERNEL);
	if (!i2c) {
		printk("error: hbs1632-i2c kmalloc\n");
		return NULL;
	}

	i2c->scl = scl;
	i2c->sda = sda;
	i2c->interval = interval > 0 ? interval : 1;

	mutex_init(&i2c->lock);
	mutex_lock(&i2c->lock);
	ret = gpio_request(i2c->scl, "hbs1632-scl");
	if (ret) {
		printk("error: hbs1632-i2c gpio_request %d\n", i2c->scl);
		goto free_i2c;
	}

	ret = gpio_request(i2c->sda, "hbs1632-sda");
	if (ret) {
		printk("error: hbs1632-i2c gpio_request %d\n", i2c->sda);
		goto free_scl;
	}
	mutex_unlock(&i2c->lock);

	printk("info: hbs1632-i2c scl %d, sda %d, interval %d\n", sda, scl, interval);

	return i2c;

free_scl:
	gpio_free(i2c->scl);
free_i2c:
	mutex_unlock(&i2c->lock);
	kfree(i2c);
	return NULL;
}
EXPORT_SYMBOL(hbs1632_i2c_create);

void hbs1632_i2c_destroy(struct hbs1632_i2c *i2c)
{
	if (!i2c)
		return;

	gpio_free(i2c->scl);
	gpio_free(i2c->sda);
	mutex_destroy(&i2c->lock);
	kfree(i2c);
}
EXPORT_SYMBOL(hbs1632_i2c_destroy);

static void i2c_start(struct hbs1632_i2c *i2c)
{
	gpio_direction_output(i2c->sda, 1);
	gpio_direction_output(i2c->scl, 1);
	udelay(i2c->interval);
	gpio_direction_output(i2c->sda, 0);
	udelay(i2c->interval);
	gpio_direction_output(i2c->scl, 0);
	udelay(i2c->interval);
}

static void i2c_stop(struct hbs1632_i2c *i2c)
{
	gpio_direction_output(i2c->scl, 0);
	gpio_direction_output(i2c->sda, 0);
	udelay(i2c->interval);
	gpio_direction_output(i2c->scl, 1);
	udelay(i2c->interval);
	gpio_direction_output(i2c->sda, 1);
	udelay(i2c->interval);
}

static int i2c_nack(struct hbs1632_i2c *i2c)
{
	u8 retries = 5;

	gpio_direction_output(i2c->scl, 0);
	gpio_direction_input(i2c->sda);
	udelay(i2c->interval);
	gpio_direction_output(i2c->scl, 1);
	udelay(i2c->interval);
	while (gpio_get_value(i2c->sda) && (retries-- > 0)) {
		gpio_direction_output(i2c->scl, 0);
		udelay(i2c->interval);
	}

	return retries > 0;
}

static void i2c_write(struct hbs1632_i2c *i2c, u8 data)
{
	u8 i;

	for (i = 0; i < 8; i++) {
		gpio_direction_output(i2c->scl, 0);
		udelay(i2c->interval);
		gpio_direction_output(i2c->sda, data & 0x80);
		udelay(i2c->interval);
		gpio_direction_output(i2c->scl, 1);
		udelay(i2c->interval);

		data <<= 1;
	}
}

static int i2c_bulk_write(struct hbs1632_i2c *i2c, const u8 *data, u8 len)
{
	u8 i;
	int nack = 0;

#ifdef DEBUG
	printk("tag-n hbs1632-i2c write %d data(s): ", len);
#endif
	for (i = 0; i < len; i++) {
#ifdef DEBUG
		printk("%02x ", data[i]);
#endif
		i2c_write(i2c, data[i]);
		nack += i2c_nack(i2c);
	}
#ifdef DEBUG
	printk("\n");
#endif

	return nack;
}

int hbs1632_i2c_write(struct hbs1632_i2c *i2c, const u8 *data, u8 len)
{
	int nack;

	mutex_lock(&i2c->lock);
	i2c_start(i2c);
	i2c_write(i2c, HBS1632_ADDR);
	nack = i2c_nack(i2c);
	if (nack) {
		// printk("warning: %s NACK\n", __FUNCTION__);
	}
	nack = i2c_bulk_write(i2c, data, len);
	i2c_stop(i2c);
	mutex_unlock(&i2c->lock);

	return nack;
}
EXPORT_SYMBOL(hbs1632_i2c_write);

MODULE_AUTHOR("Gofftang <gofftang@gmail.com>");
MODULE_DESCRIPTION("GPIO I2C driver");
MODULE_LICENSE("GPL");
