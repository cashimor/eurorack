"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Independent Windows Cellz updater. Obtain official firmware separately.
inspect is read-only; flash/migrate print an offline plan unless --execute is given.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
from plans import Plan
from legacy_transfer import Plan as LegacyPlan, LegacyTransfer
from hid_transfer import VerifiedStagingTransfer

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command',required=True)
    commands.add_parser('inspect',help='Read identity and available status; no writes')
    flash = commands.add_parser('flash',help='Plan or execute an application update')
    flash.add_argument('--firmware',type=Path,required=True)
    flash.add_argument('--manifest',type=Path,help='Required for locally built custom firmware')
    flash.add_argument('--execute',action='store_true')
    migration = commands.add_parser('migrate',help='One-time old-loader migration; tested revision 001b only')
    migration.add_argument('--installer',type=Path,required=True)
    migration.add_argument('--execute',action='store_true')
    args = parser.parse_args()
    print('Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment. Use at your own risk.',flush=True)
    if args.command == 'inspect':
        from cellz_usb import Cellz
        print(json.dumps(Cellz().inspect(),indent=2))
        return
    plan = (Plan.from_file(args.firmware,args.manifest) if args.command == 'flash'
            else LegacyPlan.official_migration(args.installer))
    signature = None
    if args.command == 'flash' and args.manifest:
        signature = json.loads(args.manifest.read_text()).get('application_signature', 'HARMONY1')
        if signature not in ('HARMONY1', 'HARMONY2', 'HARMONY3', 'HARMONY4', 'HARMONY5'):
            raise ValueError('Unsupported Harmony application signature')
        signature = signature.encode('ascii')
    summary = {'command':args.command,'source_sha256':plan.source_sha256,'blocks':len(plan.blocks)}
    if args.command == 'flash':
        summary['checksum'] = f'{plan.checksum:04x}'
        if signature:
            summary['application_signature'] = signature.decode('ascii')
    if not args.execute:
        print(json.dumps({'scope':'Offline plan only; no USB access',**summary},indent=2))
        return
    from cellz_usb import Cellz
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    directory = ROOT / 'logs'
    directory.mkdir(exist_ok=True)
    path = directory / f'{args.command}-{stamp}.json'
    journal = {'plan':summary,'success':False,'events':[]}

    def event(phase,detail=None):
        journal['events'].append({'phase':phase,'detail':detail})
        temporary = path.with_suffix('.tmp')
        temporary.write_text(json.dumps(journal,indent=2)+'\n',encoding='utf8')
        temporary.replace(path)
        print(phase,flush=True)

    print(f'Local journal: {path}',flush=True)
    engine = None
    try:
        cellz = Cellz(write=True)
        event('initial_identity',cellz.identity)
        if args.command == 'migrate':
            event('migration_started')
            result = cellz.migrate(plan,LegacyTransfer)
            event('installer_started',result)
            print(result['next_step'])
        else:
            event('loader_confirmed',cellz.enter_hid())
            transport = cellz.features(write=True)
            engine = VerifiedStagingTransfer(transport,plan)
            try:
                event('application_transfer_started')
                event('checksum_accepted',engine.transfer(plan))
                event('application_return_requested')
                engine.launch()
            finally:
                transport.close()
            event('application_returned',cellz.application(signature))
        journal['success'] = True
        event('complete')
    except Exception as exc:
        if engine is not None:
            event('transfer_events_before_stop',engine.events)
        event('stopped_without_automatic_retry_or_return',str(exc))
        raise


if __name__ == '__main__':
    main()
