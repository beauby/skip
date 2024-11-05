import type { Json, Entry, ReactiveResponse } from "@skipruntime/api";
import { parseReactiveResponse } from "./utils.js";

export type Entrypoint = {
  host: string;
  port: number;
  secured?: boolean;
};

function toHttp(entrypoint: Entrypoint) {
  if (entrypoint.secured)
    return `https://${entrypoint.host}:${entrypoint.port}`;
  return `http://${entrypoint.host}:${entrypoint.port}`;
}

export async function fetchJSON<V>(
  url: string,
  method: "POST" | "GET" | "PUT" | "PATCH" | "HEAD" | "DELETE" = "GET",
  headers: Record<string, string> = {},
  data?: Json,
): Promise<[V | null, Headers]> {
  const body = data ? JSON.stringify(data) : undefined;
  const response = await fetch(url, {
    method,
    body,
    headers: {
      "Content-Type": "application/json",
      Accept: "application/json",
      ...headers,
    },
    signal: AbortSignal.timeout(1000),
  });
  if (!response.ok) {
    throw new Error(`${response.status}: ${response.statusText}`);
  }
  const responseText = await response.text();
  const responseJSON =
    responseText.length > 0 && responseText != response.statusText
      ? JSON.parse(responseText)
      : null;
  return [responseJSON, response.headers];
}

export class RESTWrapperOfSkipService {
  private entrypoint: string;

  constructor(
    entrypoint: Entrypoint = {
      host: "localhost",
      port: 3587,
    },
  ) {
    this.entrypoint = toHttp(entrypoint);
  }

  async getAll<K extends Json, V extends Json>(
    resource: string,
    params: Record<string, string>,
    reactiveAuth?: Uint8Array | string,
  ): Promise<{ values: Entry<K, V>[]; reactive?: ReactiveResponse }> {
    const qParams = new URLSearchParams(params).toString();
    let header = {};
    if (reactiveAuth) {
      if (typeof reactiveAuth == "string") {
        header = {
          "Skip-Reactive-Auth": reactiveAuth,
        };
      } else {
        header = {
          "Skip-Reactive-Auth": Buffer.from(reactiveAuth.buffer).toString(
            "base64",
          ),
        };
      }
    }
    const [optValues, headers] = await fetchJSON<Entry<K, V>[]>(
      `${this.entrypoint}/v1/${resource}?${qParams}`,
      "GET",
      header,
    );
    const reactive = parseReactiveResponse(headers);
    const values = optValues ?? [];
    return reactive ? { values, reactive } : { values };
  }

  async head(
    resource: string,
    params: Record<string, string>,
    reactiveAuth: Uint8Array | string,
  ): Promise<ReactiveResponse> {
    const qParams = new URLSearchParams(params).toString();
    const header = this.header(reactiveAuth);
    const [_data, headers] = await fetchJSON(
      `${this.entrypoint}/v1/${resource}?${qParams}`,
      "HEAD",
      header,
    );
    const reactiveResponse = parseReactiveResponse(headers);
    if (!reactiveResponse)
      throw new Error("Reactive response must be suplied.");
    else {
      return reactiveResponse;
    }
  }

  async getArray<V extends Json>(
    resource: string,
    params: Record<string, string>,
    key: string,
    reactiveAuth?: Uint8Array | string,
  ): Promise<V[]> {
    const qParams = new URLSearchParams(params).toString();
    const header = this.header(reactiveAuth);
    const [data, _headers] = await fetchJSON<V[]>(
      `${this.entrypoint}/v1/${resource}/${key}?${qParams}`,
      "GET",
      header,
    );
    return data ?? [];
  }

  async put<K extends Json, V extends Json>(
    collection: string,
    key: K,
    value: V[],
  ): Promise<void> {
    return await this.patch(collection, [[key, value]]);
  }

  async patch<K extends Json, V extends Json>(
    collection: string,
    values: Entry<K, V>[],
  ): Promise<void> {
    await fetchJSON(`${this.entrypoint}/v1/${collection}`, "PATCH", {}, values);
  }

  async deleteKey<K extends Json>(collection: string, key: K): Promise<void> {
    return await this.patch(collection, [[key, []]]);
  }

  private header(reactiveAuth?: Uint8Array | string): Record<string, string> {
    if (reactiveAuth) {
      if (typeof reactiveAuth == "string") {
        return {
          "Skip-Reactive-Auth": reactiveAuth,
        };
      } else {
        return {
          "Skip-Reactive-Auth": Buffer.from(reactiveAuth.buffer).toString(
            "base64",
          ),
        };
      }
    }
    return {};
  }
}
