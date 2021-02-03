local uci = require("luci.model.uci").cursor()
local dsp = require "luci.dispatcher"
local m ,s, o

m = Map("mqtt_topics")

s = m:section(TypedSection, "topic", translate("Topics"), translate("") )
s.addremove = true
s.anonymous = true
s.template = "cbi/tblsection"
s.novaluetext = "There are no topics created yet."
s.main_toggled = true
s.extedit = dsp.build_url("admin", "services", "mqtt", "subscriber", "topics", "%s")

s.create = function(self, section)
	stat = TypedSection.create(self, section)
	uci:set(self.config, stat, "topic", "")
	uci:set(self.config, stat, "id", get_top_unq_id())
	uci:set(self.config, stat, "qos", "0")
	uci:set(self.config, stat, "want_retained", "0")
	luci.http.redirect(dsp.build_url("admin", "services", "mqtt", "subscriber", "topics", stat))
	return stat
end

s.remove = function(self, section)
	local id = self.map:get(section, "id")
	self.map.uci:foreach("mqtt_events", "event", function(e)
		if id == e.t_id then
			m.uci:delete("mqtt_events", e[".name"])
		end
    end)
	uci:delete("mqtt_topics", section)
	return true
end

o = s:option( DummyValue, "topic", translate("Topic name"))
o.datatype = "string"

s:option(Flag, "enabled")

function get_top_unq_id()
    local indices = {}
    m.uci:foreach("mqtt_topics", "topic", function(s)
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

