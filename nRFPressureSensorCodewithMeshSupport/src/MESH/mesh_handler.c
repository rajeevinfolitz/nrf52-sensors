#include <bluetooth/mesh/models.h>
#include <mesh_handler.h>
#define BT_MESH_SENSOR_TYPE_PRESSURE 0x01

static double dummy_ambient_light_value;
static bool attention;
static struct k_work_delayable attention_blink_work;





static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
		BIT(3),
#endif
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		//dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static struct bt_mesh_sensor_setting amb_light_level_setting[] = {
	{
		.type = &bt_mesh_sensor_gain,
		.get = NULL,
		.set = NULL,
	},
	{
		.type = &bt_mesh_sensor_present_amb_light_level,
		.get = NULL,
		.set = NULL,
	},
};

struct settings_handler amb_light_level_gain_conf = {
	.name = "amb_light_level",
	.h_set = NULL
};




static struct bt_mesh_sensor present_amb_light_level = {
	.type = &bt_mesh_sensor_pressure,
	.get = NULL,
	.descriptor = NULL,
	.settings = {
		.list = (const struct bt_mesh_sensor_setting *)&amb_light_level_setting,
		.count = ARRAY_SIZE(amb_light_level_setting),
	},
};


static struct bt_mesh_sensor *const ambient_light_sensor[] = {
	&present_amb_light_level,
};

static struct bt_mesh_sensor_srv ambient_light_sensor_srv =
	BT_MESH_SENSOR_SRV_INIT(ambient_light_sensor, ARRAY_SIZE(ambient_light_sensor));


static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}
static void attention_off(struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};


BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv,
								 &health_pub),
					BT_MESH_MODEL_SENSOR_SRV(&ambient_light_sensor_srv)),
		     BT_MESH_MODEL_NONE),
	// BT_MESH_ELEM(2,
	// 	     BT_MESH_MODEL_LIST(BT_MESH_MODEL_SENSOR_SRV(&occupancy_sensor_srv)),
	// 	     BT_MESH_MODEL_NONE),
	// BT_MESH_ELEM(3,
	// 	     BT_MESH_MODEL_LIST(BT_MESH_MODEL_SENSOR_SRV(&chip_temp_sensor_srv)),
	// 	     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = 0x0001,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};
int startPub(double unPressureRaw)
{
	int nError = 0;
	struct sensor_value val;
	dummy_ambient_light_value = 5.0;
		

		nError = sensor_value_from_double(&val, unPressureRaw);
		if (nError) {
			printk("Error getting ambient light level sensor data (%d)\n", nError);
		}

		nError = bt_mesh_sensor_srv_pub(&ambient_light_sensor_srv, NULL,
					     &present_amb_light_level, &val);
}
const struct bt_mesh_comp *model_handler_init(void)
{
	 k_work_init_delayable(&attention_blink_work, attention_blink);
	// k_work_init_delayable(&presence_detected_work, presence_detected);

	// if (!device_is_ready(dev)) {
	// 	printk("Temperature sensor not ready\n");
	// } else {
	// 	printk("Temperature sensor (%s) initiated\n", dev->name);
	// }

	//dk_button_handler_add(&button_handler);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_subsys_init();
		settings_register(&amb_light_level_gain_conf);
		//settings_register(&presence_motion_threshold_conf);
		//settings_register(&amb_light_level_gain_conf);
	}

	return &comp;
}