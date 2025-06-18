import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_FONT_ID = "font_id"
CONF_DISPLAY_ID = "display_id"

custom_ble_list_display_ns = cg.esphome_ns.namespace('custom_ble_list_display')
CustomBLEListDisplay = custom_ble_list_display_ns.class_(
    'CustomBLEListDisplay', cg.Component
)

# Import correct types
font_ns = cg.esphome_ns.namespace('font')
st7789v_ns = cg.esphome_ns.namespace('st7789v')
Font = font_ns.class_('Font')
ST7789V = st7789v_ns.class_('ST7789V')

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(CustomBLEListDisplay),
    cv.Required(CONF_FONT_ID): cv.use_id(Font),
    cv.Required(CONF_DISPLAY_ID): cv.use_id(ST7789V),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    font = await cg.get_variable(config[CONF_FONT_ID])
    display_obj = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_font(font))
    cg.add(var.set_display(display_obj))
