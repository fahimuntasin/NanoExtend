#include "ota.h"

#include <SHA2Builder.h>
#include <Update.h>
#include <esp_ota_ops.h>

#include "logger.h"
#include "system.h"

bool OtaManager::busy_ = false;
int OtaManager::progress_ = 0;
size_t OtaManager::written_ = 0;
size_t OtaManager::total_ = 0;
char OtaManager::status_[64] = "idle";
char OtaManager::expectedSha256_[65] = {0};
char OtaManager::computedSha256_[65] = {0};
bool OtaManager::pendingVerification_ = false;

namespace {
SHA256Builder sha256;

bool isSha256(const char* value) {
  if (!value || strlen(value) != 64)
    return false;
  for (size_t i = 0; i < 64; ++i) {
    if (!isxdigit(static_cast<unsigned char>(value[i])))
      return false;
  }
  return true;
}
} // namespace

void OtaManager::begin() {
  busy_ = false;
  progress_ = 0;
  written_ = 0;
  total_ = 0;
  expectedSha256_[0] = '\0';
  computedSha256_[0] = '\0';
  strncpy(status_, "idle", sizeof(status_) - 1);

  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
  if (running && esp_ota_get_state_partition(running, &state) == ESP_OK &&
      state == ESP_OTA_IMG_PENDING_VERIFY) {
    pendingVerification_ = true;
    strncpy(status_, "pending_verify", sizeof(status_) - 1);
    LOG_W("OTA", "New image pending stable-boot verification");
  } else {
    pendingVerification_ = false;
  }
  LOG_I("OTA", "Manager ready freeSketch=%u", ESP.getFreeSketchSpace());
}

bool OtaManager::isBusy() {
  return busy_;
}
int OtaManager::progress() {
  return progress_;
}
const char* OtaManager::status() {
  return status_;
}
const char* OtaManager::computedSha256() {
  return computedSha256_;
}
bool OtaManager::pendingVerification() {
  return pendingVerification_;
}

void OtaManager::setExpectedSha256(const char* sha256Value) {
  expectedSha256_[0] = '\0';
  if (!isSha256(sha256Value))
    return;
  for (size_t i = 0; i < 64; ++i) {
    expectedSha256_[i] = static_cast<char>(tolower(static_cast<unsigned char>(sha256Value[i])));
  }
  expectedSha256_[64] = '\0';
}

bool OtaManager::beginUpdate(size_t contentLength) {
  if (busy_) {
    strncpy(status_, "busy", sizeof(status_) - 1);
    return false;
  }
  if (contentLength == 0 || contentLength > ESP.getFreeSketchSpace()) {
    strncpy(status_, "too_large", sizeof(status_) - 1);
    SystemInfo::setLastError("OTA image too large");
    return false;
  }
  if (!Update.begin(contentLength)) {
    strncpy(status_, "begin_failed", sizeof(status_) - 1);
    SystemInfo::setLastError("OTA begin failed");
    return false;
  }
  busy_ = true;
  written_ = 0;
  total_ = contentLength;
  progress_ = 0;
  computedSha256_[0] = '\0';
  sha256.begin();
  strncpy(status_, "writing", sizeof(status_) - 1);
  LOG_I("OTA", "Begin update size=%u", static_cast<unsigned>(contentLength));
  return true;
}

bool OtaManager::writeChunk(const uint8_t* data, size_t len) {
  if (!busy_)
    return false;
  sha256.add(data, len);
  size_t w = Update.write(const_cast<uint8_t*>(data), len);
  if (w != len) {
    abortUpdate("write_error");
    return false;
  }
  written_ += w;
  if (total_ > 0)
    progress_ = static_cast<int>((written_ * 100) / total_);
  return true;
}

bool OtaManager::endUpdate(bool success) {
  if (!busy_)
    return false;
  if (!success) {
    abortUpdate("aborted");
    return false;
  }

  sha256.calculate();
  String digest = sha256.toString();
  strncpy(computedSha256_, digest.c_str(), sizeof(computedSha256_) - 1);
  computedSha256_[sizeof(computedSha256_) - 1] = '\0';
  if (expectedSha256_[0] != '\0' && strcasecmp(expectedSha256_, computedSha256_) != 0) {
    LOG_E("OTA", "SHA-256 mismatch expected=%s actual=%s", expectedSha256_, computedSha256_);
    abortUpdate("checksum_mismatch");
    return false;
  }

  if (!Update.end(true)) {
    abortUpdate("end_failed");
    return false;
  }
  busy_ = false;
  progress_ = 100;
  strncpy(status_, "success", sizeof(status_) - 1);
  LOG_I("OTA", "Update success; reboot required");
  return true;
}

bool OtaManager::confirmRunningImage() {
  if (!pendingVerification_)
    return true;
  esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
  if (err != ESP_OK) {
    LOG_E("OTA", "Failed to confirm running image err=0x%x", static_cast<unsigned>(err));
    return false;
  }
  pendingVerification_ = false;
  strncpy(status_, "verified", sizeof(status_) - 1);
  LOG_I("OTA", "Running image confirmed after stable boot");
  return true;
}

void OtaManager::abortUpdate(const char* reason) {
  Update.abort();
  busy_ = false;
  progress_ = 0;
  written_ = 0;
  total_ = 0;
  strncpy(status_, reason ? reason : "aborted", sizeof(status_) - 1);
  status_[sizeof(status_) - 1] = '\0';
  SystemInfo::setLastError(status_);
  LOG_E("OTA", "Aborted: %s", status_);
}

void OtaManager::fillJson(JsonObject obj) {
  obj["busy"] = busy_;
  obj["progress"] = progress_;
  obj["status"] = status_;
  obj["written"] = written_;
  obj["total"] = total_;
  obj["freeSketch"] = ESP.getFreeSketchSpace();
  obj["expectedSha256"] = expectedSha256_;
  obj["computedSha256"] = computedSha256_;
  obj["pendingVerification"] = pendingVerification_;
}
