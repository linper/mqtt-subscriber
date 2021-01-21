local uci = require("luci.model.uci").cursor()
local dsp = require "luci.dispatcher"
-- local utils = require "luci.tools.utils"
local m ,s, o

m = Map("mqtt_sub")

s = m:section(TypedSection, "topic", translate("Topics"), translate("") )
s.addremove = true
s.anonymous = true
s.template = "cbi/tblsection"
s.novaluetext = "There are no topics created yet."
s.main_toggled = true
s.extedit = dsp.build_url("admin", "services", "mqtt", "subscriber", "topics", "%s")

function s.create(self)
	stat = TypedSection.create(self, name)
	uci:set(self.config, stat, "topic", "")
	uci:set(self.config, stat, "id", get_top_unq_id())
	uci:set(self.config, stat, "qos", "0")
	uci:set(self.config, stat, "want_retained", "0")
	luci.http.redirect(dsp.build_url("admin", "services", "mqtt", "subscriber", "topics", stat))
	return stat
end

o = s:option( DummyValue, "topic", translate("Topic name"))
o.datatype = "string"

s:option(Flag, "enabled")

function get_top_unq_id()
    local indices = {}
    m.uci:foreach("mqtt_sub", "topic", function(s)
		indices[#indices + 1] = tonumber(s.id)
    end)
    local max = 0
	for i, v in pairs(indices) do
		if v > max then
			max = v
		end
    end
    return max + 1
end

return m

