import { runService } from "@skipruntime/server";
import { service } from "./dist/service.js";
import process from "process";

const platform = process.argv[2];
console.log(`Running with ${platform} backend.`);
runService(service, {
  streaming_port: 8080,
  control_port: 8081,
  platform,
});
