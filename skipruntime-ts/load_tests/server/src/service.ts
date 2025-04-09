import type {
  EagerCollection,
  Resource,
  SkipService,
} from "@skipruntime/core";

type FooServiceInputs = {
  foo: EagerCollection<string, string>;
};

class FooResource implements Resource<FooServiceInputs> {
  instantiate(collections: FooServiceInputs): EagerCollection<string, string> {
    return collections.foo;
  }
}

export const service: SkipService<FooServiceInputs, FooServiceInputs> = {
  initialData: {
    foo: [],
  },
  resources: { foo: FooResource },
  createGraph(inputs: FooServiceInputs): FooServiceInputs {
    return inputs;
  },
};
