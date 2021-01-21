module("luci.controller.mosquitto", package.seeall)

function index()

  entry( { "admin", "services", "mqtt"}, firstchild(), _("MQTT"), 150)
  entry( { "admin", "services", "mqtt", "broker" }, cbi("mqtt_broker"), _("Broker"), 1).leaf = true
  entry( { "admin", "services", "mqtt", "publisher" }, cbi("mqtt_pub"), _("Publisher"), 2).leaf = true
  entry( { "admin", "services", "mqtt", "subscriber" }, firstchild(), _("Subscriber"), 3)
  entry( { "admin", "services", "mqtt", "subscriber", "connection" }, cbi("mqtt_sub/connection"), _("Connection"), 1).leaf = true
  entry( { "admin", "services", "mqtt", "subscriber", "topics" }, arcombine(cbi("mqtt_sub/topics"), cbi("mqtt_sub/topic")), _("Topics"), 2).leaf = true
  entry( { "admin", "services", "mqtt", "subscriber", "events" }, arcombine(cbi("mqtt_sub/events"), cbi("mqtt_sub/event")), _("Events"), 3).leaf = true
  entry( { "admin", "services", "mqtt", "subscriber", "output" }, cbi("mqtt_sub/output"), _("Output"), 4).leaf = true
end
