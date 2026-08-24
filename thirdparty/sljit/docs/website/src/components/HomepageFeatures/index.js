import clsx from 'clsx';
import Heading from '@theme/Heading';
import { Icon } from '@iconify/react';
import styles from './styles.module.css';

const FeatureList = [
  {
    title: 'Versatile',
    icon: 'solar:cpu-linear',
    description: (
      <>
        SLJIT can generate code for a variety of architectures, including
        <code>x86</code>, <code>ARM</code> and <code>RISC-V</code>.
      </>
    ),
  },
  {
    title: 'Powerful',
    icon: 'solar:bolt-linear',
    description: (
      <>
        SLJIT supports a large number of operations, from basic integer math
        to tail calls and SIMD.
      </>
    ),
  },
  {
    title: 'Permissive',
    icon: 'solar:chat-square-like-linear',
    description: (
      <>
        Published under a Simplified BSD License, SLJIT can be used with
        minimal restrictions.
      </>
    ),
  },
];

function Feature({icon, title, description}) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center">
        <Icon icon={icon} height="150px" className={clsx('text', styles.featureIcon)} />
      </div>
      <div className="text--center padding-horiz--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
