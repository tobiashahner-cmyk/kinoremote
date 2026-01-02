#include "KinoMacroActions.h"
#include "KinoAPI.h"


// === Dispatcher ===


// {"dev":"yamaha","cmd":"set","val":{"volume":-600}}
// {"dev":"canvas","cmd":"set","val":{"effect":42,"bri":128,"speed":60,"palette":5},"commit":true}
ActionResult MacroActions::execute(const JsonObject& action, bool testing) {
  // validity check
  if (!action.containsKey("dev")) {
    return { ActionError::MissingCmd, "missing device" };
  }
  if (!action.containsKey("cmd")) {
    return { ActionError::MissingCmd, "missing command" };
  }
  // parameter initialisation
  const char* dev = action["dev"];
  const char* cmd = action["cmd"] | "set";
  char paramname[32];
  KinoVariant paramval;
  bool commit = action["commit"] | false;
  KinoError executionError;
  
  JsonObject value = action["val"].as<JsonObject>();
  if (testing) {
    // debug block:
    //Serial.print(value.size()); Serial.println(" Aktionen in diesem Schritt");
    for (JsonPair jp : value) {
      strncpy(paramname,jp.key().c_str(),32);
      paramval = KinoVariant::fromJsonVariant(jp.value());
      //if (strcmp(dev,"set")==0) {
        Serial.print("(DEBUG) : KinoAPI::");
        Serial.print(cmd);
        Serial.print("(\""); Serial.print(dev); Serial.print("\", \""); Serial.print(paramname); Serial.print("\", "); Serial.print(paramval.toString()); Serial.println(")");
      //}
      yield();
      delay(1);
    }
    if (commit) Serial.printf("(DEBUG) : KinoAPI::commit(\"%s\")\n",dev); 
    return {ActionError::OK,""};
  }
  if (!testing) {
    // execution block (testing = false)
    for (JsonPair jp : value) {
      strncpy(paramname,jp.key().c_str(),32);
      paramval = KinoVariant::fromJsonVariant(jp.value());
      /*
      Serial.print("(DEBUG) : KinoAPI::");
      Serial.print(cmd);
      Serial.print("(\""); Serial.print(dev); Serial.print("\", \""); Serial.print(paramname); Serial.print("\", "); Serial.print(paramval.toString()); Serial.println(")");
      */
      if (strcmp(cmd,"set")==0) {
        executionError = KinoAPI::setProperty(dev,paramname,paramval);
        if (executionError != KinoError::OK) {
          Serial.printf("Error in Macro: KinoAPI::setProperty(%s,%s,%s) failed\n",dev,paramname,paramval.toString());
          return { ActionError::ExecutionFailed, kinoErrorToString(executionError) };
        }
        yield();
        delay(1);
      } else {
        return { ActionError::UnknownCommand, cmd };
      }
    }
    if (commit) KinoAPI::commit(dev);
    return { ActionError::OK, "" };
  }
  return { ActionError::OK, "" }; 
}


String MacroActions::translateErrorCode(ActionError errCode) {
  String readable;
  switch (errCode) {
    case ActionError::MissingCmd :
      readable = "MissingCmd";
      break;
    case ActionError::MissingParam :
      readable = "MissingParam";
      break;
    case ActionError::InvalidParamType :
      readable = "InvalidParamType";
      break;
    case ActionError::ParamOutOfRange :
      readable = "ParamOutOfRange";
      break;
    case ActionError::UnknownCommand :
      readable = "UnknownCommand";
      break;
    case ActionError::ExecutionFailed :
      readable = "ExecutionFailed";
      break;
    case ActionError::DeviceNotReady :
      readable = "DeviceNotReady";
      break;
    case ActionError::Timeout :
      readable = "Timeout";
      break;
    case ActionError::InternalError :
      readable = "InternalError";
      break;
    default:
      readable = "unknown error";
      break;
  }
  return readable;
}
