import time
import prelude

sensor = prelude.Sensor("test")

alert = prelude.IDMEFAlert()

alert["alert.classification(0).name"] = "test python"
alert["alert.assessment.impact.severity"] = "low"
alert["alert.assessment.impact.completion"] = "failed"
alert["alert.assessment.impact.type"] = "recon"
alert["alert.detect_time"] = time.time()
alert["alert.source(0).node.address(0).address"] = "10.0.0.1"
alert["alert.target(0).node.address(1).address"] = "10.0.0.2"
alert["alert.target(1).node.address(0).address"] = "10.0.0.3"
alert["alert.additional_data(0).data"] = "something"

sensor.send_alert(alert)
