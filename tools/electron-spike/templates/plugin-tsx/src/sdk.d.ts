/** API v1 subset used by this template. Full SDK: plugins/sdk.ts in the host. */
export interface PluginContext {
 readonly system:string;
 ui:{registerPanel(id:string,mount:(context:{element:HTMLElement})=>void|(()=>void)):()=>void;registerCommand(id:string,run:()=>void|Promise<void>):()=>void;openPanel(id:string):void};
}
