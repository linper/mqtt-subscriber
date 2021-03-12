local sqlite = require "luci.model.sqlite".init()


local db_path = "/usr/share/mqtt_sub/mqtt_sub.db"
local data = {}

local db = sqlite.database(db_path)

if db then
	data = db:select("select * from logs order by timestamp desc;")
	db:close()
end

local m, s, o

m = Map("mqtt_sub")
m.submit = false
m.reset = false

local s = m:section(Table, data, translate("Log"), translate("The Log section contains information about received messages and MQTT subscriber client events."))
s.anonymous = true
s.template = "cbi/tblsection_dynamic"
s.addremove = false
s.refresh = true
s.table_config = {
	truncatePager = false,
	labels = {
		perPage = "Records per page {select}",
		noRows = "No entries found",
		info = ""
	},
	layout = {
		top = "<table><tr style='padding: 0 !important; border:none !important'><td style='display: flex !important; flex-direction: row'>{select}<span style='margin-left: auto; width:100px'></span></td></tr></table>",
		bottom = "{info}{pager}"
	}
}

s:option(DummyValue, "timestamp", translate("date"))
s:option(DummyValue, "topic", translate("Topic"))
s:option(DummyValue, "message", translate("Message"))

return m
