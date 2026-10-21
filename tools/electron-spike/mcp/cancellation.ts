import { randomUUID } from 'node:crypto';
import { AsyncLocalStorage } from 'node:async_hooks';
import { McpServer, type ServerContext } from '@modelcontextprotocol/server';
import type { LocalServiceClient } from '../services/local-endpoint.js';
import { ServiceError, type SessionIdentity } from '../services/contracts.js';

// Each concurrent tools/call has its own cancellation scope. Abort only an
// operation started by that call, retaining its original session/epoch and ID.
// In particular, aborting operation_status must never cancel the polled handle.
export function installCancellation(server: McpServer, client: LocalServiceClient) {
  const scope = new AsyncLocalStorage<{signal:AbortSignal; operation?:{id:number;identity:SessionIdentity}; batch?:{identity:SessionIdentity;token:string}; cancel?:Promise<unknown>}>();
  const request = client.request.bind(client);
  function cancel(s: NonNullable<ReturnType<typeof scope.getStore>>) {
    if(s.batch && !s.cancel) s.cancel=request('experiment.cancelBatch',s.batch.identity,{token:s.batch.token}).catch(()=>undefined);
    if(s.operation && !s.cancel) s.cancel = request('execution.cancel',s.operation.identity,{operationId:s.operation.id}).catch(()=>undefined);
  }
  client.request = (async (method: any, identity?: SessionIdentity, params?: any) => {
    const s=scope.getStore();
    // An aborted call cannot start additional work after an intervening await.
    if(s?.signal.aborted && !['execution.status','execution.cancel'].includes(method)) throw new ServiceError('cancelled');
    if(s && method === 'experiment.batch' && identity) {s.batch={identity,token:randomUUID()};params={...params,cancellationToken:s.batch.token};}
    const reply=await request(method,identity,params);
    if(s && method === 'experiment.batch') s.batch=undefined;
    const result=reply.ok ? reply.result as any : undefined;
    if(s && result?.state === 'running' && Number.isSafeInteger(result.operation_id) && !['execution.status','execution.cancel'].includes(method)) {
      s.operation={id:result.operation_id,identity:{session:reply.session,epoch:reply.epoch}};
      if(s.signal.aborted) cancel(s);
    }
    return reply;
  }) as typeof client.request;
  const register=server.registerTool.bind(server);
  // Keep SDK schema inference at callers; only the common runtime envelope is wrapped.
  server.registerTool = ((name:string,config:any,handler:any) => register(name,config,async (args:any,ctx:ServerContext) => {
    const s={signal:ctx.mcpReq.signal} as NonNullable<ReturnType<typeof scope.getStore>>;
    const abort=()=>cancel(s);s.signal.addEventListener('abort',abort,{once:true});
    try { return await scope.run(s,()=>handler(args,ctx)); }
    finally {s.signal.removeEventListener('abort',abort);if(s.signal.aborted){cancel(s);await s.cancel;}}
  })) as typeof server.registerTool;
}
